# GOC Tracker

## 关于GOC Tracker

* GOC Tracker 是一个能够将GOC中区块，交易等信息以网页的形式呈现出来的前端项目。其通过向GOC节点发送请求获取信息，如http://127.0.0.1:8888/v1/chain/get_info ，返回的数据是 
```
{
    "server_version": "9f1e9986",
    "chain_id": "1c6ae7719a2a3b4ecb19584a30ff510ba1b6ded86e1fd8b8fc22f1179c622a32",
    "head_block_num": 90,
    "last_irreversible_block_num": 86,
    "last_irreversible_block_id": "0000005679d999b73020adcf35f101fdc8ed451c06322c93415de923cb7e1d9d",
    "head_block_id": "0000005af02b280f424b2dbcb2ee0cd4b4d63343039f2921f962ca3a07a512ed",
    "head_block_time": "2018-10-11T04:03:37.500",
    "head_block_producer": "producer111g",
    "virtual_block_cpu_limit": 218581,
    "virtual_block_net_limit": 1146181,
    "block_cpu_limit": 99900,
    "block_net_limit": 1048576,
    "server_version_string": "v1.3.0-60-g9f1e99869-dirty"
}
```

* 具体EOS的API信息可以参考[官方文档](https://developers.eos.io/eosio-nodeos/reference)，GOC相关的API会根据需求再开发,参考[链接](https://github.com/BlockchainLabFudan/SeaOfKnowledge/blob/master/goc/goc_rpc_api/README.md)。

* GOC Tracker 是fork自[EOSTracker](https://github.com/EOSEssentials/EOSTracker)

## 配置环境

* 下载GOC Tracker 代码，执行 git clone [https://github.com/BlockchainLabFudan/GOCTracker.git](https://github.com/BlockchainLabFudan/GOCTracker.git)

* 安装nodejs, npm, angular
  * apt-get install nodejs
  * apt-get install npm  #注意要求 nodejs 8.x 和 npm 5.x 以上版本，老版本可能会出现错误。nodejs -v, npm -v 可以查看版本。
  * npm install -g @angular/cli

* 安装GOC Tracker 依赖的模块
  * cd GOCTracker
  * npm install

## 运行

* 连接到GOC节点
  * 修改/GOCTracker/src/environments/environment.ts 中的blockchainUrl: 'https://eos.greymass.com'
  * 将地址改为运行GOC节点的,如blockchainUrl: 'http://goc.gravitypool.io:8080'
  * 上述Url是在启动节点时使用--http-server-address arg配置

* 运行
  * cd GOCTracker
  * ng serve [--host 192.168.1.102 --port 4200] #host和port可以修改
  * 在浏览器中输入上述网址就可以看到页面了

* 注意事项
  * 为了支持GOC Tracker发起的跨域请求，提供服务的节点需要配置--access-control-allow-origin '*'
  * 为了使用当前GOC Tracker的完整功能，提供服务的节点需要enable以下plugin（可能不完整，部分enable可能有风险）
  ```
  --plugin eosio::http_plugin
  --plugin eosio::chain_api_plugin
  --plugin eosio::producer_plugin
  --plugin eosio::history_plugin
  --plugin eosio::history_api_plugin
  --plugin eosio::producer_api_plugin
  --plugin eosio::net_plugin
  --plugin eosio::net_api_plugin
  ```
