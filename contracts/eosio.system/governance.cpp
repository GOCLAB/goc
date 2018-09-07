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

void system_contract::gocstake(account_name payer)
{
    require_auth(payer);
    auto time_now = now();

    asset actual_quant = asset(_gstate.goc_stake_limit);

    user_resources_table userres(_self, payer);
    auto res_itr = userres.find(payer);
    if (res_itr == userres.end())
    {
        //no record for stake 
        res_itr = userres.emplace(payer, [&](auto &res) {
            res.owner = payer;
            res.governance_stake = actual_quant;
            res.goc_stake_freeze = time_now;
        });
    }
    else
    {
        //has record and no enough stake 
        if (res_itr->governance_stake >= asset(0) && res_itr->governance_stake < asset(_gstate.goc_stake_limit))
        {
            actual_quant = asset(_gstate.goc_stake_limit) - res_itr->governance_stake;
            userres.modify(res_itr, payer, [&](auto &res) {
                res.governance_stake += asset(actual_quant);
                if (res_itr->goc_stake_freeze < time_now)
                    res.goc_stake_freeze = time_now;
            });
        } else {
            actual_quant = asset(0);
        }
    }

    if(actual_quant > asset(0))
        INLINE_ACTION_SENDER(eosio::token, transfer)
        (N(eosio.token), {payer, N(active)},
        {payer, N(eosio.gstake), actual_quant, std::string("stake goc")});
}

void system_contract::gocunstake(account_name receiver)
{
    require_auth(receiver);
    auto time_now = now();

    user_resources_table userres(_self, receiver);
    auto res_itr = userres.find(receiver);

    eosio_assert(res_itr != userres.end(), "no resource row");
    eosio_assert(res_itr->goc_stake_freeze < time_now, "stake frozen");

    asset tokens_out = res_itr->governance_stake;
    userres.modify(res_itr, receiver, [&](auto &res) {
        res.governance_stake = asset(0);
    });

    INLINE_ACTION_SENDER(eosio::token, transfer)
    (N(eosio.token), {N(eosio.gstake), N(active)},
    {N(eosio.gstake), receiver, tokens_out, std::string("unstake GOC")});
}

void system_contract::gocnewprop(const account_name owner, asset fee, const std::string &pname, const std::string &pcontent, const std::string &url, uint16_t start_type)
{
    require_auth(owner);
    auto time_now = now();

    eosio_assert(pname.size() < 512, "name too long");
    eosio_assert(pcontent.size() < 1024, "content too long");
    eosio_assert(url.size() < 512, "url too long");
    eosio_assert(fee.symbol == asset().symbol, "fee must be system token");
    eosio_assert(fee.amount > _gstate.goc_proposal_fee_limit, "insufficient fee");

    //charge proposal fee to goc gn saving account
    INLINE_ACTION_SENDER(eosio::token, transfer)
    (N(eosio.token), {owner, N(active)},
     {owner, N(eosio.gocgns), fee, std::string("create proposal")});

    uint64_t new_id = _gocproposals.available_primary_key();
    //create proposal
    _gocproposals.emplace(owner, [&](auto &info) {
        info.id = new_id;
        info.owner = owner;
        info.fee = fee;
        info.proposal_name = pname;
        info.proposal_content = pcontent;
        info.url = url;
        info.create_time = time_now;
        info.vote_starttime = time_now + _gstate.goc_vote_start_time;
        info.bp_vote_starttime = time_now + _gstate.goc_vote_start_time + _gstate.goc_governance_vote_period;
        info.bp_vote_endtime = time_now + _gstate.goc_vote_start_time + _gstate.goc_governance_vote_period + _gstate.goc_bp_vote_period;

        if (start_type == 1)
            info.vote_starttime = time_now;

        if (start_type == 2)
        { // TEST ONLY
            info.vote_starttime = time_now;
            info.bp_vote_starttime = time_now;
        }
        info.total_yeas = 0;
        info.total_nays = 0;
        info.bp_nays = 0;
    });

    eosio::print("created propsal ID: ", new_id, "\n");
}

void system_contract::gocupprop(const account_name owner, uint64_t id, const std::string &pname, const std::string &pcontent, const std::string &url)
{
    require_auth(owner);

    eosio_assert(pname.size() < 512, "name too long");
    eosio_assert(pcontent.size() < 1024, "content too long");
    eosio_assert(url.size() < 512, "url too long");

    const auto &proposal_updating = _gocproposals.get(id, "proposal not exist");

    eosio_assert(proposal_updating.vote_starttime > now(), "proposal is not available for modify");
    eosio_assert(proposal_updating.owner == owner, "only owner can update proposal");

    //charge little for avoid attacking
    asset fee = asset(_gstate.goc_action_fee, CORE_SYMBOL);
    INLINE_ACTION_SENDER(eosio::token, transfer)
    (N(eosio.token), {owner, N(active)},
     {owner, N(eosio.gocgns), fee, std::string("update proposal")});

    _gocproposals.modify(proposal_updating, 0, [&](auto &info) {
        info.proposal_name = pname;
        info.proposal_content = pcontent;
        info.url = url;
    });
}

void system_contract::gocvote(account_name voter, uint64_t pid, bool yea)
{
    require_auth(voter);

    const auto &proposal_voting = _gocproposals.get(pid, "proposal not exist");

    auto time_now = now();

    //User vote time, bp vote as normal user
    eosio_assert(proposal_voting.vote_starttime < time_now, "proposal voting not yet started”");
    eosio_assert(proposal_voting.bp_vote_starttime > time_now, "proposal voting expired");

    //User need stake for vote

    user_resources_table userres(_self, voter);
    auto res_itr = userres.find(voter);

    eosio_assert(res_itr != userres.end(), "no resource row");
    eosio_assert(res_itr->governance_stake.amount >= _gstate.goc_stake_limit, "insufficient stake for voting");

    //TODO:maybe need fee to run for avoid attacking

    //the votes table for pid
    goc_votes_table votes(_self, pid);

    auto vote_info = votes.find(voter);

    if (vote_info != votes.end())
    {
        bool old_vote = vote_info->vote;

        if (yea != old_vote)
        {
            eosio::print("updating old vote of ", name{voter}, " ", pid, " ", yea, "\n");

            votes.modify(vote_info, 0, [&](auto &info) {
                info.vote = yea;
                info.vote_update_time = time_now;
            });

            _gocproposals.modify(proposal_voting, 0, [&](auto &info) {
                if (yea)
                {
                    info.total_nays -= 1.0;
                    info.total_yeas += 1.0;
                }
                else
                {
                    info.total_nays += 1.0;
                    info.total_yeas -= 1.0;
                }
            });
        }
        else
        {
            eosio::print("skip same vote of ", name{voter}, "\n");
        }
    }
    else
    {
        eosio::print("creating vote for ", name{voter}, " ", pid, " ", yea, "\n");

        votes.emplace(voter, [&](auto &info) {
            info.owner = voter;
            info.vote = yea;
            info.vote_time = time_now;
            info.vote_update_time = time_now;
        });
        _gocproposals.modify(proposal_voting, 0, [&](auto &info) {
            if (yea)
            {
                info.total_yeas += 1.0;
            }
            else
            {
                info.total_nays += 1.0;
            }
        });
        //freeze goc stake to bp vote end
        userres.modify(res_itr, voter, [&](auto &res) {
            res.goc_stake_freeze = proposal_voting.bp_vote_starttime + _gstate.goc_bp_vote_period;
        });
    }
}

void system_contract::gocbpvote(account_name voter, uint64_t pid, bool yea)
{
    require_auth(voter);

    auto time_now = now();

    const auto &proposal_voting = _gocproposals.get(pid, "proposal not exist");

    //BP vote time, only bp is allowed, not need for stake.

    auto idx = _producers.get_index<N(prototalvote)>();

    uint16_t bp_count = 0;
    for (auto it = idx.cbegin(); it != idx.cend() && bp_count < 21 && 0 < it->total_votes && it->active(); ++it)
    {
        if (it->owner == voter)
            break;
        bp_count++;
    }

    eosio_assert(bp_count < 21, "only bp can vote");

    //bp vote time window
    eosio_assert(proposal_voting.bp_vote_starttime < time_now, "proposal bp voting not yet started");
    eosio_assert(proposal_voting.bp_vote_endtime > time_now, "proposal bp voting expired");

    //TODO:maybe need fee to run for avoid attacking

    //the bp votes table for pid
    goc_bp_votes_table bpvotes(_self, pid);

    auto vote_info = bpvotes.find(voter);

    if (vote_info != bpvotes.end())
    {
        bool old_vote = vote_info->vote;

        if (yea != old_vote)
        {
            eosio::print("updating old bp vote of ", name{voter}, " ", pid, " ", yea, "\n");

            bpvotes.modify(vote_info, 0, [&](auto &info) {
                info.vote = yea;
                info.vote_update_time = time_now;
            });

            _gocproposals.modify(proposal_voting, 0, [&](auto &info) {
                if (yea)
                {
                    info.bp_nays -= 1.0;
                }
                else
                {
                    info.bp_nays += 1.0;
                }
            });
        }
        else
        {
            eosio::print("skip same bp vote of ", name{voter}, "\n");
        }
    }
    else
    {
        eosio::print("creating bp vote for ", name{voter}, " ", pid, " ", yea, "\n");

        bpvotes.emplace(voter, [&](goc_vote_info &info) {
            info.owner = voter;
            info.vote = yea;
            info.vote_time = time_now;
            info.vote_update_time = time_now;
        });
        _gocproposals.modify(proposal_voting, 0, [&](auto &info) {
            if (yea)
            {
                info.bp_nays -= 1.0;
            }
            else
            {
                info.bp_nays += 1.0;
            }
        });
    }
}

} // namespace eosiosystem
