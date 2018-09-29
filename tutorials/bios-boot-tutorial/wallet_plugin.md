# Wallet_plugin_分析

wallet_api_plugin需要wallet_plugin和http_plugin

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

## 3.Wallet_plugin

### Wallet_manager

#### 整体功能：

``` sh
 Provides associate of wallet name to wallet and manages the interaction with each wallet.

 The name of the wallet is also used as part of the file name by soft_wallet. See wallet_manager::create.
 No const methods because timeout may cause lock_all() to be called.
```

#### 分函数分析：

set_dir  设置钱包文件的路径，并初始化lock文件，如果lock文件打开失败或者被占用则报错。传入路径

set_timeout  设置锁定所有钱包的超时时间，不活动(任何的wallet_manager方法)达到超时时间则lock_all()。传入秒数

sign_transaction  通过公钥对应的私钥签名交易，其中使用chain_controller::get_required_keys来确定交易需要什么密钥。需要传入：需要签名的交易，相应的公钥，其上的chain_id。返回已签名的交易。当没在已解锁的钱包里找到对应私钥的时候抛出例外。

sign_digest  用公钥签名摘要。需要传入：需要签名的摘要，相应公钥。返回已签名的摘要。当没在已解锁的钱包里找到对应私钥的时候抛出例外。

create  在设定路径下创建一个新的钱包。传入name

open  打开路径下的指定钱包。传入name

list_wallets，list_keys，get_public_keys分别返回钱包、私钥和公钥。其中：钱包名字前带*表示已解锁；返回私钥需要传入钱包的名字和密码。返回公钥范围为已解锁的钱包。

lock_all，lock，unlock为一系列锁定与解锁钱包操作。

import_key  导入私钥到特定钱包中，私钥为WIF格式，钱包必须处于开启解锁状态。传入钱包名和wif格式的private-key

remove_key  从钱包中移除密钥对，钱包必须处于开启解锁状态。传入钱包名、密码和公钥。当钱包未找到、未解锁或者公钥未找到的情况下抛出例外。

create_key  生成一对公私钥，钱包必须处于开启解锁状态。传入钱包名和公私钥对的格式。当钱包未找到、未解锁或者无法生成指定格式的密钥对时抛出异常。返回公钥

own_and_use_wallet  应为新加入钱包的操作，当钱包名重复的时候抛出例外。

check_timeout  根据chrono的系统时钟判断是否超时，是的话调用lock_all()

initialize_lock  初始化lock文件，如果lock文件打开失败或者被占用则报错

### Wallet（准确来说是soft wallet）

另有yubihsm-wallet和se-wallet，其中yubihsm为硬件钱包，se与其类似，使用apple的一款叫做Secure Enclave的硬件，但具体如何链接和使用se未清楚。这两种功能都比soft wallet少，只有钱包密码、锁定和密钥等相关，应该是将其他功能集成在了硬件上

#### 整体功能：

``` sh
 This wallet assumes it is connected to the database server with a high-bandwidth, low-latency connection and performs minimal caching.
 ```

#### 分函数分析：

copy_wallet_file  备份wallet

get_wallet_filename  返回当前的钱包名称，自动保存钱包时使用

get_private_key  获取公钥对应的WIF格式的私钥，私钥必须已经在钱包中

get_private_key_from_password  从账户+角色+密码生成种子，使用种子的sha256hash值来生成私钥。返回公私钥对。角色有 active | owner | posting | memo  

is_new  如果钱包里还没有导入任何一个密钥，则返回true

islocked,lock,unlock  同上

check_password  检查密码是否正确，即使钱包已经解锁了，如果密码不正确还是会抛出错误

set_password  当钱包是new的时候，或者unlock状态下才能调用，设置新密码

list_keys  需要解锁钱包，返回所有公私钥对（WIF格式）

list_plublic_keys  需要解锁钱包，返回所有公钥

load_wallet_file  载入一个特定的石墨烯钱包，现在的钱包将被关闭，如果不传入文件名则reload现在的钱包

save_wallet_file  以新名字保存钱包，序列化存储，另存一个副本而不是保存。不传入文件名则保存现在的文件名

set_wallet_filename  仅仅改变钱包文件名，以备以后使用

import_key，remove_key  同上

create_key  传入key_type，例如K1, R1

try_sign_digest  试图通过给定的公钥签名摘要

plain_keys  结构，内有sha512类型的校验和和一对公私钥对

encrypt_keys  使用校验和作为对称密钥， aes加密序列化的plain_keys

### Wallet_plugin

需要wallet_manager支持

#### set_program_options  

设置选项：  
  钱包路径（绝对或相对于应用路径），默认为当前路径下
  超时时间，默认900秒，即15分钟
  yubihsm-url，覆盖默认的“http://localhost:12345”来连接硬件钱包
  yubihsm-authkey，传入Authkey来enable YubiHsm支持

#### plugin_initialize

初始化插件  
  根据设置选项的配置通过wallet_manager初始化各种参数
  如果传入了yubihsm-authkey，则根据yubihsm-url打开新钱包

## 4.进一步读源码

### - Wallet_manager

- 钱包的有效名字为字母|数字|._-，不是这几类的都无效
- 初始timeout_time被设为max_time，check是否超时的时候判断timeout_time是否等于max_time即可
- create一个新钱包之后会进行unlock-lock-unlock的test环节
- create新钱包后可能会出现，原先存在重名的wallet文件在keosd运行的时候被删除，在这里eos预先判断是否在wallet表里存在同名wallet，若存在则先将其移除
  同样的，open新钱包也会出现相似情况，因为可能在keosd运行的时候添加了一个新钱包文件
- sign_transaction传入多个公钥的时候需要返回所有公钥对交易的签名，而sign_digest只能传入一个公钥

### - 整体架构

wallet_api_plugin 通过http_plugin设置回调函数，用户访问http的url，触发wallet_manager的回调函数，wallet_manager的函数调用了wallet_api基类（只有hpp文件）的函数，底层的三种wallet(se、soft、yubihsm)都是继承基类产生的。因此三者文件函数名都类似，只是做了多层封装。wallet_manager更多地是对所有的钱包进行处理，而soft_wallet更多针对单个钱包的处理实现。