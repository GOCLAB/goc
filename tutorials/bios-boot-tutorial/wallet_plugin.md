# Wallet_plugin_分析
需要wallet_plugin和http_plugin
## 1.Wallet_命令行参数

``` sh
Interact with local wallet
Usage: ../../build/programs/cleos/cleos wallet SUBCOMMAND

Subcommands:
  create                      Create a new wallet locally
  open                        Open an existing wallet
  lock                        Lock wallet
  lock_all                    Lock all unlocked wallets
  unlock                      Unlock wallet
  import                      Import private key into wallet
  remove_key                  Remove key from wallet
  create_key                  Create private key within wallet
  list                        List opened wallets, * = unlocked
  keys                        List of public keys from all unlocked wallets.
  private_keys                List of private keys from an unlocked wallet in wif or PVT_R1 format.
  stop                        Stop keosd (doesnot work with nodeos.
```

## 2.Wallet_api_plugin

### call命令调用http_plugin

#### 输入api_name,，api_handle,，call_name，INVOKE_xxx， http_response_code 调用

- api_name:本例中为wallet

- api_handle:本例中为wallet_mgr

- call_name:(无视重复的call_name和api_handle,只看传入参数和返回码

  ``` sh
  set_timeout: INVOKE_V_R, int64_t, 200
  sign_transaction: INVOKE_R_R_R_R, chain::signed_transaction, flat_set<public_key_type>, chain:chain_id_type, 201
  sign_digest: INVOKE_R_R_R, chain::digest_type, public_key_type, 201
  create: INVOKE_R_R, std::string, 201
  open: INVOKE_V_R, std::string, 200
  lock_all: INVOKE_V_V, 200
  lock: INVOKE_V_R, std::string, 200
  unlock: INVOKE_V_R_R, std::string, std::string, 200
  import_key: INVOKE_V_R_R, std::string, std::string, 201
  remove_key: INVOKE_V_R_R_R, std::string, std::string, std::string, 201
  create_key: INVOKE_R_R_R, std::string, std::string, 201
  list_wallets: INVOKE_R_V, 200
  list_keys: INVOKE_R_R_R, std::string, std::string, 200
  get_public_keys: INVOKE_R_V, 200
  ```

- INVOKE_xxx   具体见上，vr、rv等的区别猜测,vr不需要返回值,rv不需要传入参数,rr有返回值和传入参数,vv都不需要

- http_response_code： 见上

### http_plugin初始化

需要http_plugin可以被获取，如果http和https没有监听端点或监听端点为回环地址，直接成功。如果没有https支持，且http不满足要求，打印错误log。如果http满足要求，打印警告log

## Wallet_plugin
