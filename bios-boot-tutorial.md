# steps 

`tutorials/bios-boot-tutorial/bios-boot-tutorial.py`

```js
commands = [
    ('k', 'kill',           stepKillAll,                True,    "Kill all nodeos and keosd processes"),
    ('w', 'wallet',         stepStartWallet,            True,    "Start keosd, create wallet, fill with keys"),
    ('W', 'new-wallet',     stepStartNewWallet,         False,   "Empty Data, Start keosd, create wallet, fill with keys"),
    ('b', 'boot',           stepStartBoot,              True,    "Start boot node"),
    ('s', 'sys',            createSystemAccounts,       True,    "Create system accounts (gocio.*)"),
    ('c', 'contracts',      stepInstallSystemContracts, True,    "Install system contracts (token, msig)"),
    ('t', 'tokens',         stepCreateTokens,           True,    "Create tokens"),
    ('S', 'sys-contract',   stepSetSystemContract,      True,    "Set system contract"),
    ('T', 'stake',          stepCreateStakedAccounts,   True,    "Create staked accounts"),
    ('g', 'goc',            stepGOC,                    True,    "Prepare goc data"),  
    ('p', 'reg-prod',       stepRegProducers,           True,    "Register producers"),
    ('P', 'start-prod',     stepStartProducers,         True,    "Start producers"),
    ('v', 'vote',           stepVote,                   True,    "Vote for producers"),
    ('R', 'claim',          claimRewards,               True,    "Claim rewards"),
    ('x', 'proxy',          stepProxyVotes,             True,    "Proxy votes"),
    ('q', 'resign',         stepResign,                 True,    "Resign eosio"),
    ('m', 'msg-replace',    msigReplaceSystem,          False,   "Replace system contract using msig"),
    ('X', 'xfer',           stepTransfer,               False,   "Random transfer tokens (infinite loop)"),
    ('l', 'log',            stepLog,                    True,    "Show tail of node's log"),
]
```
## 配置设置

```js
// eosio 默认账户的公钥私钥
parser.add_argument('--public-key', metavar='', help="EOSIO Public Key", default='GOC8Znrtgwt8TfpmbVpTKvA2oB8Nqey625CLN8bCN3TEbgx86Dsvr', dest="public_key")
parser.add_argument('--private-Key', metavar='', help="EOSIO Private Key", default='5K463ynhZoCDDa4RDcr63cUwWLTnKqmdcoTKTHBjqoKfv4u5V7p', dest="private_key")
// cleos 路径和 keosd 钱包端口
parser.add_argument('--cleos', metavar='', help="Cleos command", default='../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 ')
parser.add_argument('--nodeos', metavar='', help="Path to nodeos binary", default='../../build/programs/nodeos/nodeos')
parser.add_argument('--keosd', metavar='', help="Path to keosd binary", default='../../build/programs/keosd/keosd')
// 智能合约路径, 各种路径
parser.add_argument('--contracts-dir', metavar='', help="Path to contracts directory", default='../../build/contracts/')
parser.add_argument('--nodes-dir', metavar='', help="Path to nodes directory", default='./nodes/')
parser.add_argument('--genesis', metavar='', help="Path to genesis.json", default="./genesis.json")
parser.add_argument('--wallet-dir', metavar='', help="Path to wallet directory", default='./wallet/')
parser.add_argument('--log-path', metavar='', help="Path to log file", default='./output.log')
parser.add_argument('--symbol', metavar='', help="The gocio.system symbol", default='SYS')
// 导入钱包的最大用户数量
parser.add_argument('--user-limit', metavar='', help="Max number of users. (0 = no limit)", type=int, default=50)
// 导入钱包的最大公钥数量(同一个公钥可以给多个用户名使用)
parser.add_argument('--max-user-keys', metavar='', help="Maximum user keys to import into wallet", type=int, default=50)
parser.add_argument('--ram-funds', metavar='', help="How much funds for each user to spend on ram", type=float, default=0.2)
parser.add_argument('--min-stake', metavar='', help="Minimum stake before allocating unstaked funds", type=float, default=0.9)
parser.add_argument('--max-unstaked', metavar='', help="Maximum unstaked funds", type=float, default=1000)
// 超级节点的开启数量, 建议小一点
parser.add_argument('--producer-limit', metavar='', help="Maximum number of producers. (0 = no limit)", type=int, default=9)
parser.add_argument('--min-producer-funds', metavar='', help="Minimum producer funds", type=float, default=1000.0000)
// 模拟投票时一个用户所投的节点的个数
parser.add_argument('--num-producers-vote', metavar='', help="Number of producers for which each user votes", type=int, default=4)
// 参与投票的用户数量
parser.add_argument('--num-voters', metavar='', help="Number of voters", type=int, default=50)
// 随机交易函数中参与交易的用户数量
parser.add_argument('--num-senders', metavar='', help="Number of users to transfer funds randomly", type=int, default=10)
parser.add_argument('--producer-sync-delay', metavar='', help="Time (s) to sleep to allow producers to sync", type=int, default=80)
// 全部运行
parser.add_argument('-a', '--all', action='store_true', help="Do everything marked with (*)")
parser.add_argument('-H', '--http-port', type=int, default=8000, metavar='', help='HTTP port for cleos')
```

## 单步执行的指令

1. ('k', 'kill',           stepKillAll,                True,    "Kill all nodeos and keosd processes"),

    > killall keosd nodeos || true

2. ('w', 'wallet',         stepStartWallet,            True,    "Start keosd, create wallet, fill with keys"),

    > rm -rf /home/halo/Desktop/dev/eos-Dev/GOCint/tutorials/bios-boot-tutorial/wallet

    > mkdir -p /home/halo/Desktop/dev/eos-Dev/GOCint/tutorials/bios-boot-tutorial/wallet

    > ../../build/programs/keosd/keosd --unlock-timeout 999999999 --http-server-address 127.0.0.1:6666 --wallet-dir /home/halo/Desktop/dev/eos-Dev/GOCint/tutorials/bios-boot-tutorial/wallet
    
    > ../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8000 wallet create --to-console

    loop
    
    > ../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8000 wallet import --private-key 5K463ynhZoCDDa4RDcr63cUwWLTnKqmdcoTKTHBjqoKfv4u5V7p

3. ('b', 'boot',           stepStartBoot,              True,    "Start boot node"),
    
    > rm -rf ./nodes/00-eosio/

    > mkdir -p ./nodes/00-eosio/

    > ../../build/programs/nodeos/nodeos    --max-irreversible-block-age -1    --contracts-console    --genesis-json /home/halo/Desktop/dev/eos-Dev/GOCint/tutorials/bios-boot-tutorial/genesis.json    --blocks-dir /home/halo/Desktop/dev/eos-Dev/GOCint/tutorials/bios-boot-tutorial/nodes/00-eosio/blocks    --config-dir /home/halo/Desktop/dev/eos-Dev/GOCint/tutorials/bios-boot-tutorial/nodes/00-eosio    --data-dir /home/halo/Desktop/dev/eos-Dev/GOCint/tutorials/bios-boot-tutorial/nodes/00-eosio    --chain-state-db-size-mb 1024    --http-server-address 127.0.0.1:8000    --p2p-listen-endpoint 127.0.0.1:9000    --max-clients 14    --p2p-max-nodes-per-host 14    --enable-stale-production    --max-transaction-time=1000    --producer-name eosio    --signature-provider=GOC8Znrtgwt8TfpmbVpTKvA2oB8Nqey625CLN8bCN3TEbgx86Dsvr=KEY:5K463ynhZoCDDa4RDcr63cUwWLTnKqmdcoTKTHBjqoKfv4u5V7p    --verbose-http-errors    --plugin eosio::http_plugin    --plugin eosio::chain_api_plugin    --plugin eosio::producer_plugin    --plugin eosio::history_plugin    --plugin eosio::history_api_plugin    2>>./nodes/00-eosio/stderr

4. ('s', 'sys',            createSystemAccounts,       True,    "Create system accounts (gocio.*)"),

    loop

    same key for all

    > ../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8000 create account eosio gocio.bpay GOC8Znrtgwt8TfpmbVpTKvA2oB8Nqey625CLN8bCN3TEbgx86Dsvr

    ```python
    systemAccounts = [
        'gocio.bpay',
        'gocio.msig',
        'gocio.names',
        'gocio.ram',
        'gocio.ramfee',
        'gocio.saving',
        'gocio.stake',
        'gocio.token',
        'gocio.vpay',
        'gocio.gocgns',
        'gocio.gstake',
        'gocio.gocvs',
    ]
    ```
5. ('c', 'contracts',      stepInstallSystemContracts, True,    "Install system contracts (token, msig)"),

    > ../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8000 set contract gocio.token ../../build/contracts/gocio.token/

    > ../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8000 set contract gocio.msig ../../build/contracts/gocio.msig/

    > 

6. ('t', 'tokens',         stepCreateTokens,           True,    "Create tokens"),

    > ../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8000 push action gocio.token create '["gocio", "10000000000.0000 SYS"]' -p gocio.token

    > ../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8000 push action gocio.token issue '["gocio", "999999999.9998 SYS", "memo"]' -p eosio

7. ('S', 'sys-contract',   stepSetSystemContract,      True,    "Set system contract"),

    > ../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8000 set contract eosio ../../build/contracts/eosio.system/

    > ../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8000 push action eosio setpriv '["gocio.msig", 1]' -p gocio@active

8. ('T', 'stake',          stepCreateStakedAccounts,   True,    "Create staked accounts"),

    loop

    > `useraaaaaaaa: total funds=209634957.2734 SYS, ram=0.2000 SYS, net=104816978.5367 SYS, cpu=104816978.5367 SYS, unstaked=1000.0000 SYS`

    > ../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8000 system newaccount --transfer eosio useraaaaaaaa GOC69X3383RzBZj41k73CSjUNXM5MYGpnDxyPnWUKPEtYQmTBWz4D --stake-net "104816978.5367 SYS" --stake-cpu "104816978.5367 SYS" --buy-ram "0.2000 SYS"   

    > ../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8000 transfer eosio useraaaaaaaa "1000.0000 SYS

    ```python
    userlist = {
        users[100]
        producers[4]
    }
    ```
9. ('g', 'goc',            stepGOC,                    True,    "Prepare goc data"), 

10. ('p', 'reg-prod',       stepRegProducers,           True,    "Register producers"),

    loop in producers[4]

    > ../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8000 system regproducer producer111a GOC8imf2TDq6FKtLZ8mvXPWcd6EF2rQwo8zKdLNzsbU9EiMSt9Lwz https://producer111a.com/GOC8imf2TDq6FKtLZ8mvXPWcd6EF2rQwo8zKdLNzsbU9EiMSt9Lwz

    > ../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8000 get table eosio eosio producers -l 30

11. ('P', 'start-prod',     stepStartProducers,         True,    "Start producers"),
    
    loop in producers[4]

    > rm -rf ./nodes/01-producer111a/

    > mkdir -p ./nodes/01-producer111a/

    > ../../build/programs/nodeos/nodeos    --max-irreversible-block-age -1    --contracts-console    --genesis-json /home/halo/Desktop/dev/eos-Dev/GOCint/tutorials/bios-boot-tutorial/genesis.json    --blocks-dir /home/halo/Desktop/dev/eos-Dev/GOCint/tutorials/bios-boot-tutorial/nodes/01-producer111a/blocks    --config-dir /home/halo/Desktop/dev/eos-Dev/GOCint/tutorials/bios-boot-tutorial/nodes/01-producer111a    --data-dir /home/halo/Desktop/dev/eos-Dev/GOCint/tutorials/bios-boot-tutorial/nodes/01-producer111a    --chain-state-db-size-mb 1024    --http-server-address 127.0.0.1:8001    --p2p-listen-endpoint 127.0.0.1:9001    --max-clients 14    --p2p-max-nodes-per-host 14    --enable-stale-production    --max-transaction-time=1000    --producer-name producer111a    --signature-provider=GOC8imf2TDq6FKtLZ8mvXPWcd6EF2rQwo8zKdLNzsbU9EiMSt9Lwz=KEY:5KLGj1HGRWbk5xNmoKfrcrQHXvcVJBPdAckoiJgFftXSJjLPp7b    --verbose-http-errors    --plugin eosio::http_plugin    --plugin eosio::chain_api_plugin    --plugin eosio::producer_plugin    --p2p-peer-address localhost:9000    2>>./nodes/01-producer111a/stderr


12. ('v', 'vote',           stepVote,                   True,    "Vote for producers"),

    loop in voters[100]

    > ../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8000 system voteproducer prods useraaaaaaaa producer111c producer111d producer111a

    show current producer list

    > ../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8000 get table eosio eosio producers -l 30

13. ('R', 'claim',          claimRewards,               True,    "Claim rewards"),
    
    > ../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8000 get table eosio eosio producers -l 100
    
    loop in producers[...]
    
    > ../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://127.0.0.1:8000 system claimrewards -j producer111b