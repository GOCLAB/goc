# 加入远端服务器并设置本机producer教程（GOC.1011_132）

## 1.开启本地钱包服务

.\bios-boot-tutorial.py -w

## 2.生成密钥与账户并导入

../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://47.52.114.54:8000 wallet create_key --to-console  
5JSmNkiGSrsHsPVjoZYRR29GqFyhiaXrfD7pkzZoLoFjUKFccLv  
GOC6N7FuKhCoEf2NUfsmZ5Gux4fNn2gMiroMNsR6tfgUT7VxNQEV3

**centos里不需要--to-console，create_key直接导入钱包，之后需提供钱包密码使用wallet private_keys查看生成公钥的对应私钥，因此也省去了下面的导入步骤**

../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://47.52.114.54:8000 wallet import -n default --private-key 5JSmNkiGSrsHsPVjoZYRR29GqFyhiaXrfD7pkzZoLoFjUKFccLv

## 3.在服务器上建立账户

**weiyan为自定义名称，需要修改**

### 如果gocio没钱需要给它先生钱，不然会报错

../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://47.52.114.54:8000 push action gocio.token issue '["gocio", "1000000000.0003 GOC", "memo"]' -p eosio

### 生成新账户，并使用之前的公钥

../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://47.52.114.54:8000 system newaccount --transfer eosio weiyan GOC6N7FuKhCoEf2NUfsmZ5Gux4fNn2gMiroMNsR6tfgUT7VxNQEV3 --stake-net "50246.5437 GOC" --stake-cpu "50246.5437 GOC" --buy-ram "0.2000 GOC"

### 给新账户转点钱

../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://47.52.114.54:8000 transfer eosio weiyan "1000.0000 GOC"

### 让新账户注册成为producer

../../build/programs/cleos/cleos --wallet-url http://127.0.0.1:6666 --url http://47.52.114.54:8000 system regproducer weiyan GOC6N7FuKhCoEf2NUfsmZ5Gux4fNn2gMiroMNsR6tfgUT7VxNQEV3 https://weiyan.com/GOC6N7FuKhCoEf2NUfsmZ5Gux4fNn2gMiroMNsR6tfgUT7VxNQEV3

## 4.开启nodeos，开启本地端口，日志记录在stderr下

**此处需要修改对应的genesis.json，并且路径也需要修改，如果需要外部访问可以将127.0.0.1修改为0.0.0.0**

rm -rf ./nodes/weiyan/  
mkdir -p ./nodes/weiyan/  
../../build/programs/nodeos/nodeos    --max-irreversible-block-age -1    --contracts-console    --genesis-json /home/thor/Desktop/GocInt/tutorials/bios-boot-tutorial/genesis.json    --blocks-dir /home/thor/Desktop/GocInt/tutorials/bios-boot-tutorial/nodes/weiyan/blocks    --config-dir /home/thor/Desktop/GocInt/tutorials/bios-boot-tutorial/nodes/weiyan    --data-dir /home/thor/Desktop/GocInt/tutorials/bios-boot-tutorial/nodes/weiyan    --chain-state-db-size-mb 1024    --http-server-address 127.0.0.1:8000    --p2p-listen-endpoint 127.0.0.1:9000    --max-clients 19    --p2p-max-nodes-per-host 19    --enable-stale-production    --max-transaction-time=1000    --producer-name weiyan    --signature-provider=GOC6N7FuKhCoEf2NUfsmZ5Gux4fNn2gMiroMNsR6tfgUT7VxNQEV3=KEY:5JSmNkiGSrsHsPVjoZYRR29GqFyhiaXrfD7pkzZoLoFjUKFccLv    --verbose-http-errors    --plugin eosio::http_plugin    --plugin eosio::chain_api_plugin    --plugin eosio::producer_plugin    --p2p-peer-address 47.52.114.54:9000  --p2p-peer-address 47.52.114.54:9001  --p2p-peer-address 47.52.114.54:9002  --p2p-peer-address 47.52.114.54:9003    2>>./nodes/weiyan/stderr
