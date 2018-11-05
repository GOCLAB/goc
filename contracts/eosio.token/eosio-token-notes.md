# 阅读笔记： eosio.token 合约
合约功能：代币的创建 (create)、发行 (issue)、和转账 (transfer)。

## 合约使用
让我们看看在 GOC 项目的教程中，有哪些地方使用了 eosio.token。
* 部署合约

    合约需被部署到一个用户 (account) 上，教程中先利用 gocio 用户创建一个名为 gocio.token 的用户
    ```
    $ cleos create account gocio gocio.token GOC8Znr...
    # gocio <= gocio::newaccount {"creator":"gocio","name":"gocio.token","owner":{"threshold":1,"keys":[{"key":"GOC8Z ...
    ```

    把 eosio.token 合约部署到 gocio.token 这名用户上
    ```
    $ cleos set contract gocio.token eosio.token
    # gocio <= gocio::setcode {"account":"gocio.token","vmtype":0,"vmversion":0,"code":"00617...
    # gocio <= gocio::setabi {"account":"gocio.token","abi":"0e656...
    ```
* 创建代币

    调用 gocio.token 在 eosio.token 这份合约中提供的 create 的操作 (action)，此操作创建 10000000000 个 GOC 代币，而创建人为 gocio。此处 `-p gocio.token` 即使用权限 `gocio.token@active` 去进行操作
    ```
    $ cleos push action gocio.token create '["gocio", "10000000000.0000 GOC"]' -p gocio.token
    # gocio.token <= gocio.token::create {"issuer":"gocio","maximum_supply":"10000000000.0000 GOC"}
    ```

* 发行代币
    
    gocio 向自己发行 1000000000 个代币，并备注 `memo`。此处需有代币创建人 `gocio@active` 的权限
    ```
    $ cleos push action gocio.token issue '["gocio", "1000000000.0000 GOC", "memo"]' -p gocio
    # gocio.token <= gocio.token::issue {"to":"gocio","quantity":"1000000000.0000 GOC","memo":"memo"}
    ```

* 转账代币

    用户 useraaaaaaab 向 useraaaaaaae 转账 0.0001 个 GOC
    ```
    $ cleos transfer -f useraaaaaaab useraaaaaaae "0.0001 GOC"
    # gocio.token <= gocio.token::transfer {"from":"useraaaaaaab","to":"useraaaaaaae","quantity":"0.0001 GOC","memo":"1541418253580860"}
    # useraaaaaaab <= gocio.token::transfer {"from":"useraaaaaaab","to":"useraaaaaaae","quantity":"0.0001 GOC","memo":"1541418253580860"}
    # useraaaaaaae <= gocio.token::transfer {"from":"useraaaaaaab","to":"useraaaaaaae","quantity":"0.0001 GOC","memo":"1541418253580860"}
    ```
    其中 `-f` 通过在 `meno` 写入随机 nonce (此次随机到 1541418253580860) 来实现每笔交易的 uniqueness，即可被区分的交易。此处我们注意到 `gocio.token::transfer` 这操作除了告知了 gocio.token，同时也通知了 useraaaaaaab 和 useraaaaaaae。此指令的执行结果等价于
    ```
    $ cleos push action gocio.token transfer '[ "useraaaaaaab", "useraaaaaaae", "0.0001 GOC", "1541418253580860" ]' -p useraaaaaaab
    ```

## 代码阅读
eosio.token 合约具体实现于 `eosio.token.hpp` 和 `eosio.token.cpp`。 `eosio.token.hpp` 中定义了 `eosio::token` 类。 `token` 是 `contract` 的子类，而它的 constructor 接收一个 `account_name` 类别的合约拥有人。 `token` 有2个数据表`multi_index<N(accounts), account> accounts` 和`multi_index<N(stat), currency_stats> stats`，以及5个成员方法(class methods)，包括`create` ， `issue`， `transfer`， `get_supply` 及`get_balance`。此处我们先给出一些结构定义：

```
typedef uint64_t account_name; // unsigned long long type

struct asset {
    int64_t amount; // long long type
    symbol_type symbol; // 代币简写 e.g. GOC, EOS
};

struct account {
    asset balance;
    uint64_t primary_key()const { return balance.symbol.name(); }
};

struct currency_stats {
    asset supply;
    asset max_supply;
    account_name issuer;
    uint64_t primary_key()const { return supply.symbol.name(); }
};
```
下面我们分析一下每个成员方法
* token::create ( account_name issuer, asset maximum_supply )
    > 代币发行者 `issuer` 打算创建 `maximum_supply.amount` 个 `maximum_supply.symbol` 代币
    1. 需要合约拥有人权限，即在 constructor 传入的 `account_name`
    2. 查看并确认此合约拥有人的数据表 `stats` 中先前没有此 `maximum_supply.symbol` 代币
    3. 为 `issuer` 创建此代币，即在 `stats` 表中加入 (emplace) 一项数据：
        ```
        {
            s.supply.symbol = maximum_supply.symbol;
            s.max_supply = maximum_supply;
            s.issuer = issuer;
        }
        ```
* token::issue( account_name to, asset quantity, string memo )
    > issuer 打算向 `to` 发行 `quantity.amount` 个 `quantity.symbol` 代币，并备注 `memo`
    1. 查看并确认此合约拥有人的数据表 `stats` 中有此 `quantity.symbol` 代币的记录，设此记录为 `st`
    2. 需要代币发行者权限，即 `st.issuer`
    3. 确认 `quantity.amount <= st.max_supply.amount - st.supply.amount`
    4. 修改 (modify) `stats` 中的 `st` ，令 `st.supply.amount += quantity.amount`
    5. 在 `accounts` 表中为 `st.issuer` 的 balance 增加 `quantity.amount`
    6. 把 `issuer` 增加的 balance 用 inline_action 方式调用 `token::transfer` 转给 `to`

* token::transfer( account_name from, account_name to, asset quantity, string memo )
    > `from` 打算向 `to` 转交 `quantity.amount` 个 `quantity.symbol` 代币，并备注 `memo`
    1. 需要 `from` 的权限
    2. 向 `from` 和 `to` 提供 recipient，即他们会被告知该操作
    3. 在 `accounts` 表调整 `from` 和 `to` 的 balance

* token::get_supply( symbol_name sym )

    > 在 `stats` 表中查看 `sym` 代币的 supply

* token::get_balance( account_name owner, symbol_name sym )

    > 在 `accounts` 表中查看 `owner` 的 `sym` 代币的 balance

虽然类有5个方法，但后2个方法只对数据表作查找；只有首3个方法会修改数据表，因此被加入合约及 ABI。 (not sure..)
```
EOSIO_ABI( eosio::token, (create)(issue)(transfer) )
```