#include "eosio.system.hpp"

#include <eosio.token/eosio.token.hpp>

namespace eosiosystem {

   const int64_t  min_pervote_daily_pay = 100'0000;
   const int64_t  min_activated_stake   = 150'000'000'0000;
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

      require_auth(N(eosio));

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

      const asset token_supply   = token( N(eosio.token)).get_supply(symbol_type(system_token_symbol).name() );
      const auto usecs_since_last_fill = ct - _gstate.last_pervote_bucket_fill;

      if( usecs_since_last_fill > 0 && _gstate.last_pervote_bucket_fill > 0 ) {
         auto new_tokens = static_cast<int64_t>( (continuous_rate * double(token_supply.amount) * double(usecs_since_last_fill)) / double(useconds_per_year) );
         //GOC rules
         //GOC give 0.1%(20% of new tokens) for voting
         auto to_producers       = new_tokens / 5;
         //GOC give 0.5% for voting
         auto to_voters          = new_tokens / 10;
         //GOC give 0.75% for gn
         auto to_gns             = new_tokens * 3 / 20;
         //GOC save 2.75% for worker plan system 
         auto to_savings         = new_tokens - to_producers - to_voters - to_gns;

         //remain the same as EOS
         auto to_per_block_pay   = to_producers / 4;
         auto to_per_vote_pay    = to_producers - to_per_block_pay;

         INLINE_ACTION_SENDER(eosio::token, issue)( N(eosio.token), {{N(eosio),N(active)}},
                                                    {N(eosio), asset(new_tokens), std::string("issue tokens for producer pay and savings")} );

         //GOC add
         INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio),N(active)},
                                                       { N(eosio), N(eosio.gocvs), asset(to_voters), "fund voter bucket" } );
         //GOC add
         INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio),N(active)},
                                                       { N(eosio), N(eosio.gocgns), asset(to_gns), "fund gn bucket" } );                                              
         
         INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio),N(active)},
                                                       { N(eosio), N(eosio.saving), asset(to_savings), "unallocated inflation" } );

         INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio),N(active)},
                                                       { N(eosio), N(eosio.bpay), asset(to_per_block_pay), "fund per-block bucket" } );

         INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio),N(active)},
                                                       { N(eosio), N(eosio.vpay), asset(to_per_vote_pay), "fund per-vote bucket" } );

         _gstate.pervote_bucket  += to_per_vote_pay;
         _gstate.perblock_bucket += to_per_block_pay;

         //as EOS, use these buckets for tokens counting
         _gstate.goc_gn_bucket    += to_gns;
         _gstate.goc_voter_bucket += to_voters;

         _gstate.last_pervote_bucket_fill = ct;
      }

      // GOC cal gn rewards on prod's claimreward action, every 24H once 
      if(ct >= _gstate.last_gn_bucket_empty + useconds_per_day) {

        auto idx = _gocproposals.get_index<N(byendtime)>();

        std::vector<uint64_t> end_proposals;

        for ( auto it = idx.cbegin(); it != idx.cend() && _gstate.last_gn_bucket_empty <= it->bp_vote_endtime; ++it ) {
            end_proposals.push_back( it->id );
            
            idx.modify(it, 0, [&](auto& info){
                info.settle_time = time_now;
            });       
        }

        auto proposal_count = end_proposals.size();

        // have end proposals
        if (proposal_count > 0)
        {
            // Divide rewards for every passed proposal
            auto per_proposal_reward = _gstate.goc_gn_bucket / proposal_count;

            // every single proposal have max reward limit
            if (per_proposal_reward > _gstate.goc_max_proposal_reward)
                per_proposal_reward = _gstate.goc_max_proposal_reward;

            // proposal fee and rewards is divided for every gn voters
            for(uint64_t pid : end_proposals) {

                
                goc_votes_table votes(_self, pid);

                // check all vote in votes info table
                for(auto& vote : votes)
                {
                    //add reward to users pending reward table to avoid heavy load, they can use refund command to get them
                    goc_rewards_table rewards(_self, vote.owner);

                    rewards.emplace(_self, [&](auto &info){
                        info.owner = vote.owner;
                        info.reward_time = time_now;
                        info.proposal_id = pid;
                        info.rewards = asset(per_proposal_reward);
                    });

                    

                    votes.modify(vote, 0, [&](auto &vote_info){
                        vote_info.settle_time = time_now;
                    });
                }
            }

            //empty the gn bucket every time, left token saved in gocgns account
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
         INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio.bpay),N(active)},
                                                       { N(eosio.bpay), owner, asset(producer_per_block_pay), std::string("producer block pay") } );
      }
      if( producer_per_vote_pay > 0 ) {
         INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio.vpay),N(active)},
                                                       { N(eosio.vpay), owner, asset(producer_per_vote_pay), std::string("producer vote pay") } );
      }
   }

} //namespace eosiosystem
