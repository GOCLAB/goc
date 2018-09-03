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

    void system_contract::gocnewprop(const account_name owner, asset fee, const std::string& pname, const std::string& pcontent, const std::string& url)
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

        //create proposal
        _gocproposals.emplace(_self, [&](goc_proposal_info &info) {
            info.id = _gocproposals.available_primary_key();
            info.owner = owner;
            info.fee = fee;
            info.proposal_name = pname;
            info.proposal_content = pcontent;
            info.url = url;
            info.create_time = eosio::time_point_sec(now());
            //TODO: must set to zero hour of today
            info.vote_starttime = eosio::time_point_sec(now() + _gstate.goc_vote_start_time);
            info.bp_vote_starttime = eosio::time_point_sec(now() + _gstate.goc_vote_start_time + _gstate.goc_governance_vote_period);
            info.total_yeas = 0;
            info.total_nays = 0;
        });
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

        _gocproposals.modify( proposal_updating, _self, [&]( goc_proposal_info& info ) {
            info.proposal_name = pname;
            info.proposal_content = pcontent;
            info.url = url;
        });
    }

    // void system_contract::voteproposal(account_name voter, bool yea)
    // {
    //     require_auth(voter);
    // }

} // namespace eosiosystem
