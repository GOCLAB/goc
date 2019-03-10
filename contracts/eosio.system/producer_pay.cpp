#include "eosio.system.hpp"

#include <eosio.token/eosio.token.hpp>

namespace eosiosystem {

   const int64_t  min_pervote_daily_pay = 1000'0000;
   const int64_t  min_activated_stake   = 1500'000'000'0000;
   const double   continuous_rate       = 0.04879;          // 5% annual rate
   const double   perblock_rate         = 0.0025;           // 0.25%
   const double   standby_rate          = 0.0075;           // 0.75%
   const uint32_t blocks_per_year       = 52*7*24*2*3600;   // half seconds per year
   const uint32_t seconds_per_year      = 52*7*24*3600;
   const uint32_t blocks_per_day        = 2 * 24 * 3600;
   const uint32_t blocks_per_hour       = 2 * 3600;
   const uint64_t useconds_per_day      = 24 * 3600 * uint64_t(1000000);
   const uint64_t useconds_per_year     = seconds_per_year*1000000ll;

   //GOC settings, not used in code, percentage of new token. 
   const double   goc_bp_rate           = 0.2;              // 1% for BP 
   const double   goc_vote_rate         = 0.1;              // 0.5% for BP votes
   const double   goc_gn_rate           = 0.15;             // 0.75% for GN
   const double   goc_wps_rate          = 0.55;             // 2.75% for WPS



   void system_contract::onblock( block_timestamp timestamp, account_name producer ) {
      using namespace eosio;

      require_auth(N(gocio));

      /** until activated stake crosses this threshold no new rewards are paid */
      if( _gstate.total_activated_stake < min_activated_stake )
         return;

      if( _gstate.last_pervote_bucket_fill == 0 )  /// start the presses
      {
         _gstate.last_pervote_bucket_fill = current_time();
         //GOC use gn_bucket to count last day's gn rewards
         _gstate.last_gn_bucket_empty = now();
      }    


      /**
       * At startup the initial producer may not be one that is registered / elected
       * and therefore there may be no producer object for them.
       */
      auto prod = _producers.find(producer);
      if ( prod != _producers.end() ) {
         _gstate.total_unpaid_blocks++;
         _producers.modify( prod, 0, [&](auto& p ) {
               p.unpaid_blocks++;
         });
      }

      /// only update block producers once every minute, block_timestamp is in half seconds
      if( timestamp.slot - _gstate.last_producer_schedule_update.slot > 120 ) {
         update_elected_producers( timestamp );

         if( (timestamp.slot - _gstate.last_name_close.slot) > blocks_per_day ) {
            name_bid_table bids(_self,_self);
            auto idx = bids.get_index<N(highbid)>();
            auto highest = idx.begin();
            if( highest != idx.end() &&
                highest->high_bid > 0 &&
                highest->last_bid_time < (current_time() - useconds_per_day) &&
                _gstate.thresh_activated_stake_time > 0 &&
                (current_time() - _gstate.thresh_activated_stake_time) > 14 * useconds_per_day ) {
                   _gstate.last_name_close = timestamp;
                   idx.modify( highest, 0, [&]( auto& b ){
                         b.high_bid = -b.high_bid;
               });
            }
         }
      }
   }

   using namespace eosio;
   void system_contract::claimrewards( const account_name& owner ) {
      require_auth(owner);

      const auto& prod = _producers.get( owner );
      eosio_assert( prod.active(), "producer does not have an active key" );

      eosio_assert( _gstate.total_activated_stake >= min_activated_stake,
                    "cannot claim rewards until the chain is activated (at least 15% of all tokens participate in voting)" );

      auto ct = current_time();
      auto time_now = now();

      eosio_assert( ct - prod.last_claim_time > useconds_per_day, "already claimed rewards within past day" );

      const asset token_supply   = token( N(gocio.token)).get_supply(symbol_type(system_token_symbol).name() );
      const auto usecs_since_last_fill = ct - _gstate.last_pervote_bucket_fill;

      if( usecs_since_last_fill > 0 && _gstate.last_pervote_bucket_fill > 0 ) {
         auto new_tokens = static_cast<int64_t>( (continuous_rate * double(token_supply.amount) * double(usecs_since_last_fill)) / double(useconds_per_year) );
         //GOC rules
         //GOC give 1%(20% of new tokens) for voting
         auto to_producers       = new_tokens / 5;
         //GOC give 0.5%(10% of new tokens)  for voting
         auto to_voters          = new_tokens / 10;
         //GOC give 0.75%(15% of new tokens) for gn
         auto to_gns             = new_tokens * 3 / 20;
         //GOC save 2.75%(55% of new tokens) for worker plan system, means 11/20 saved
         auto to_savings         = new_tokens - to_producers - to_voters - to_gns;

         //remain the same as EOS
         auto to_per_block_pay   = to_producers / 4;
         auto to_per_vote_pay    = to_producers - to_per_block_pay;

         INLINE_ACTION_SENDER(eosio::token, issue)( N(gocio.token), {{N(gocio),N(active)}},
                                                    {N(gocio), asset(new_tokens), std::string("issue tokens for producer pay and savings")} );

         //GOC add
         INLINE_ACTION_SENDER(eosio::token, transfer)( N(gocio.token), {N(gocio),N(active)},
                                                       { N(gocio), N(gocio.vs), asset(to_voters), "fund voter bucket" } );
         //GOC add
         INLINE_ACTION_SENDER(eosio::token, transfer)( N(gocio.token), {N(gocio),N(active)},
                                                       { N(gocio), N(gocio.gns), asset(to_gns), "fund gn bucket" } );                                              
         
         INLINE_ACTION_SENDER(eosio::token, transfer)( N(gocio.token), {N(gocio),N(active)},
                                                       { N(gocio), N(gocio.saving), asset(to_savings), "unallocated inflation" } );

         INLINE_ACTION_SENDER(eosio::token, transfer)( N(gocio.token), {N(gocio),N(active)},
                                                       { N(gocio), N(gocio.bpay), asset(to_per_block_pay), "fund per-block bucket" } );

         INLINE_ACTION_SENDER(eosio::token, transfer)( N(gocio.token), {N(gocio),N(active)},
                                                       { N(gocio), N(gocio.vpay), asset(to_per_vote_pay), "fund per-vote bucket" } );

         _gstate.pervote_bucket  += to_per_vote_pay;
         _gstate.perblock_bucket += to_per_block_pay;

         //as EOS, use these buckets for tokens counting
         _gstate.goc_gn_bucket    += to_gns;
         _gstate.goc_voter_bucket += to_voters;

         _gstate.last_pervote_bucket_fill = ct;
      }

      //GOC cal vote rewards every 24H
      if (time_now >= _gstate.last_voter_bucket_empty + seconds_per_day) {

          int64_t per_stake_reward = static_cast<int64_t>(_gstate.goc_voter_bucket / _gstate.total_stake + _gstate.goc_lockbw_stake);

          // count all voters
          for(auto& voter : _voters) {
             account_name reward_to;
             int64_t reward_stake;

             if(voter.staked > 100'000'0000 || voter.proxied_vote_stake >100'000'0000) { //100 000 GOC
                //proxy will take all proxied stake reward and its own stake reward
                if(voter.proxy) {
                   reward_to = 0;
                   reward_stake = 0;
                } else {
                   reward_to = voter.owner;
                   if(voter.is_proxy)
                     reward_stake = voter.staked + voter.proxied_vote_stake;
                   else
                     reward_stake = voter.staked;
                }

               if(reward_stake > 0) {
                  goc_vote_rewards_table vrewards(_self, reward_to);

                  auto from_vreward = vrewards.find((uint64_t)0);  //find reward_id = 0 

                  if( from_vreward == vrewards.end() ) {
                     from_vreward = vrewards.emplace( _self, [&]( auto& v ) {
                        v.reward_id = 0;
                        v.reward_time  = time_now;
                        v.rewards = per_stake_reward * reward_stake;
                     });
                  } else {
                     vrewards.modify( from_vreward, 0, [&]( auto& v ) {
                        v.reward_time  = time_now;
                        v.rewards += per_stake_reward * reward_stake;
                     });
                  }
                  
               }
             }
          }

          // count all lockbw
          auto idx = _lockband.get_index<N(byendtime)>();
          for( auto it = idx.cend(); it != idx.cbegin(); ) {
             --it;
             if (time_now < it->lock_end_time) {
                idx.modify(it, 0, [&](auto& info){
                   info.reward_bucket += per_stake_reward * it->net_cpu_weight;
                });
             } else {
                break;
             }
          }

          //empty the voter bucket every time, left token saved in vs account
          _gstate.goc_voter_bucket = 0;
          //reset voter bucket empty time
          _gstate.last_voter_bucket_empty = time_now;
      }

      // GOC cal gn rewards on prod's claimreward action, every 24H * 14 once 
      if(time_now >= _gstate.last_gn_bucket_empty + seconds_per_day * 14) {

        auto idx = _gocproposals.get_index<N(byendtime)>();

        std::vector<uint64_t> end_proposals;

        //add all ended and not settled proposal to container
        for ( auto it = idx.cbegin(); it != idx.cend(); ++it ) {
            if(time_now >= it->bp_vote_endtime && it->settle_time == 0) 
            {
                end_proposals.push_back( it->id );
            
                idx.modify(it, 0, [&](auto& info){
                    info.settle_time = time_now;
                });
            }
        }

        auto proposal_count = end_proposals.size();

        // have end proposals
        if (proposal_count > 0)
        {
            // Divide rewards for every passed proposal
            int64_t per_proposal_reward = static_cast<int64_t>(_gstate.goc_gn_bucket / proposal_count);

            // every single proposal have max reward limit
            if (per_proposal_reward > _gstate.goc_max_proposal_reward)
                per_proposal_reward = _gstate.goc_max_proposal_reward;

            // proposal fee and rewards is divided for every gn voters
            for(uint64_t pid : end_proposals) {

                
                goc_votes_table votes(_self, pid);

                const auto &proposal_updating = _gocproposals.get(pid, "proposal not exist");

                uint64_t vote_count = proposal_updating.total_voter;

                //record settle time & reward
                _gocproposals.modify(proposal_updating, 0, [&](auto &info) {
                    info.settle_time = time_now;
                    info.reward = asset (per_proposal_reward);
                });
                
                int64_t vote_reward_token = 0;
                
                // only deal with voted proposal
                if(vote_count > 0.0)
                {
                    vote_reward_token = static_cast<int64_t>((double)per_proposal_reward / (double)vote_count);
                    if (vote_reward_token > _gstate.goc_max_prop_reward_per_voter)
                        vote_reward_token = _gstate.goc_max_prop_reward_per_voter;

                    // check all vote in votes info table
                    for(auto& vote : votes)
                    {
                        //update reward in user pending reward table to avoid heavy load, they can use refund command to get them
                        goc_rewards_table rewards(_self, vote.owner);

                        const auto &reward_updating = rewards.get(pid, "proposal not exist in user rewards record");

                        rewards.modify(reward_updating, 0, [&](auto &info){
                            info.reward_time = time_now;
                            //every one share proposal reward
                            info.rewards = asset(vote_reward_token);
                        });

                        votes.modify(vote, 0, [&](auto &vote_info){
                            vote_info.settle_time = time_now;
                        });
                    }
                }
            }

            //empty the gn bucket every time, left token saved in gns account
            _gstate.goc_gn_bucket =0;
            //reset gn bucket empty time
            _gstate.last_gn_bucket_empty = time_now;
        }
      }


      // here EOS count the producer's unpaid block who claimrewards
      int64_t producer_per_block_pay = 0;
      if( _gstate.total_unpaid_blocks > 0 ) {
         producer_per_block_pay = (_gstate.perblock_bucket * prod.unpaid_blocks) / _gstate.total_unpaid_blocks;
      }
      // here EOS count the producer's votes who claimrewards
      int64_t producer_per_vote_pay = 0;
      if( _gstate.total_producer_vote_weight > 0 ) {
         producer_per_vote_pay  = int64_t((_gstate.pervote_bucket * prod.total_votes ) / _gstate.total_producer_vote_weight);
      }
      if( producer_per_vote_pay < min_pervote_daily_pay ) {
         producer_per_vote_pay = 0;
      }
      _gstate.pervote_bucket      -= producer_per_vote_pay;
      _gstate.perblock_bucket     -= producer_per_block_pay;
      _gstate.total_unpaid_blocks -= prod.unpaid_blocks;

      _producers.modify( prod, 0, [&](auto& p) {
          p.last_claim_time = ct;
          p.unpaid_blocks = 0;
      });

      if( producer_per_block_pay > 0 ) {
         INLINE_ACTION_SENDER(eosio::token, transfer)( N(gocio.token), {N(gocio.bpay),N(active)},
                                                       { N(gocio.bpay), owner, asset(producer_per_block_pay), std::string("producer block pay") } );
      }
      if( producer_per_vote_pay > 0 ) {
         INLINE_ACTION_SENDER(eosio::token, transfer)( N(gocio.token), {N(gocio.vpay),N(active)},
                                                       { N(gocio.vpay), owner, asset(producer_per_vote_pay), std::string("producer vote pay") } );
      }
   }

} //namespace eosiosystem
