/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include "eosio.system.hpp"

#include <eosiolib/eosio.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/print.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/transaction.hpp>
#include <eosio.token/eosio.token.hpp>

#include <algorithm>
#include <cmath>

namespace eosiosystem
{
    using eosio::bytes;
    using eosio::const_mem_fun;
    using eosio::indexed_by;
    using eosio::print;
    using eosio::singleton;
    using eosio::transaction;

    void system_contract::gocnewprop(const account_name owner, asset fee, const std::string& pname, const std::string& pcontent, const std::string& url, uint16_t start_type)
    {
        require_auth(owner);

        eosio_assert(pname.size() < 512, "name too long");
        eosio_assert(pcontent.size() < 1024, "content too long");
        eosio_assert(url.size() < 512, "url too long");
        eosio_assert(fee.symbol == asset().symbol, "fee must be system token");
        eosio_assert(fee.amount > _gstate.goc_proposal_fee_limit, "insufficient fee");

        //charge proposal fee
        INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {owner, N(active)},
         { owner, N(eosio.gpfee), fee, std::string("create proposal") } );

        uint64_t new_id = _gocproposals.available_primary_key();

        //create proposal
        _gocproposals.emplace(owner, [&](goc_proposal_info &info) {
            info.id = new_id;
            info.owner = owner;
            info.fee = fee;
            info.proposal_name = pname;
            info.proposal_content = pcontent;
            info.url = url;
            info.create_time = eosio::time_point_sec(now());
            //TODO: must set to zero hour of today
            if( start_type == 0 )
                info.vote_starttime = eosio::time_point_sec(now() + _gstate.goc_vote_start_time);
            if( start_type == 1 )
                info.vote_starttime = eosio::time_point_sec(now());

            info.bp_vote_starttime = eosio::time_point_sec(now() + _gstate.goc_vote_start_time + _gstate.goc_governance_vote_period);
            if( start_type == 2 )
                info.bp_vote_starttime = eosio::time_point_sec(now());
            info.total_yeas = 0;
            info.total_nays = 0;
            info.bp_nays = 0;
        });

        eosio::print("created propsal ID: ", new_id, "\n");
    }

    void system_contract::gocupprop(const account_name owner, uint64_t id, const std::string& pname, const std::string& pcontent, const std::string& url)
    {
        require_auth(owner);

        eosio_assert(pname.size() < 512, "name too long");
        eosio_assert(pcontent.size() < 1024, "content too long");
        eosio_assert(url.size() < 512, "url too long");

        const auto& proposal_updating = _gocproposals.get( id , "proposal not exist");

        eosio_assert( proposal_updating.vote_starttime > eosio::time_point_sec(now()), "proposal is not available for modify" );
        eosio_assert( proposal_updating.owner == owner, "only owner can update proposal");

        //charge little for updating
        asset fee = asset(10000, CORE_SYMBOL);
        INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {owner, N(active)},
         { owner, N(eosio.gpfee), fee, std::string("update proposal") } );

        _gocproposals.modify( proposal_updating, 0, [&]( goc_proposal_info& info ) {
            info.proposal_name = pname;
            info.proposal_content = pcontent;
            info.url = url;
        });
    }

    void system_contract::gocvote(account_name voter, uint64_t pid, bool yea)
    {
        require_auth(voter);

        const auto& proposal_voting = _gocproposals.get( pid , "proposal not exist");

        eosio_assert( proposal_voting.vote_starttime < eosio::time_point_sec(now()), "proposal is not available for voting" );
        eosio_assert( proposal_voting.bp_vote_starttime > eosio::time_point_sec(now()), "proposal is not available for voting" );

        goc_votes_table votes( _self, pid );

        auto vote_info = votes.find( voter );

        if ( vote_info != votes.end() ) {
            bool old_vote = vote_info->vote;

            if(yea != old_vote) {
                eosio::print("updating old vote of ", voter, " ", pid, " ", yea, "\n");

                votes.modify( vote_info, 0, [&]( goc_vote_info& info ){
                    info.vote = yea;
                    info.vote_update_time = eosio::time_point_sec(now());
                    });

                _gocproposals.modify(proposal_voting, 0, [&]( goc_proposal_info& info ){              
                    if(yea) {
                        info.total_nays -= 1.0;
                        info.total_yeas += 1.0;
                    } else {
                        info.total_nays += 1.0;
                        info.total_yeas -= 1.0;                      
                    }
                });
             } else {
                eosio::print("skip same vote of ", voter, "\n");
             }
        } else {
            eosio::print("creating vote for ", voter, " ", pid, " ", yea, "\n");

            votes.emplace(voter, [&](goc_vote_info& info) {
                info.owner = voter;
                info.vote = yea;
                info.bp = false;
                info.vote_time = eosio::time_point_sec(now());
                info.vote_update_time = eosio::time_point_sec(now());
            });
            _gocproposals.modify(proposal_voting, 0, [&]( goc_proposal_info& info ){              
                if(yea) {
                    info.total_yeas += 1.0;
                } else {
                    info.total_nays += 1.0;
                }
            });
        }

        

    }

} // namespace eosiosystem
