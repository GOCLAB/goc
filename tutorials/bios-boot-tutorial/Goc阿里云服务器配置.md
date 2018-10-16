# Goc阿里云服务器配置

## Linux用户添加

- 新建组

  `groupadd gocTest`

- 新建用户

  `useradd -g gocTest -m czn` 

- 修改密码

  `passwd czn`

- 修改sudo文件让子用户使用sudo

  `visudo`

  `czn ALL=(ALL) ALL`

## Goc下载与配置启动

- 下载git，python36等工具（使用sudo权限下载，一个账户操作过其他账户共享）

- github上下载testnet源码并编译

  `git clone https://github.com/BlockchainLabFudan/GOCtestnet --recursive` (速度过慢可以使用rz工具从本地上传压缩包然后解压)

  `./eosio_build.sh`

- 参考GOCint中的connect_to_cloud.md创建账户并连接到测试网

- fc如果获取出错，需要配置github上的ssh获取方式，再使用`git submodule update --init --recursive`

### 部署GOCTracker

- github上下载GOCTracker源码

  `git clone https://github.com/BlockchainLabFudan/GOCTracker`

- 安装node、npm、angular等



### 注意事项

- genesis.json为：

```{
  {
  "initial_timestamp": "2018-09-09T09:09:09.999",
  "initial_key": "GOC7oiy488wfzwbR6L9QwNDQb4CHGVR5ix9udJjVyykgbLaQLgBc5",
  "initial_configuration": {
    "max_block_net_usage": 1048576,
    "target_block_net_usage_pct": 1000,
    "max_transaction_net_usage": 524288,
    "base_per_transaction_net_usage": 12,
    "net_usage_leeway": 500,
    "context_free_discount_net_usage_num": 20,
    "context_free_discount_net_usage_den": 100,
    "max_block_cpu_usage": 200000,
    "target_block_cpu_usage_pct": 1000,
    "max_transaction_cpu_usage": 150000,
    "min_transaction_cpu_usage": 100,
    "max_transaction_lifetime": 3600,
    "deferred_trx_expiration_window": 600,
    "max_transaction_delay": 3888000,
    "max_inline_action_size": 4096,
    "max_inline_action_depth": 4,
    "max_authorit``y_depth": 6
  }
}
```

- 提供的端口为8000-8003，9000-9003分别供给http和p2p使用,4200-4203给eostracker使用
- 注意修改账号为gocio，监听地址为0.0.0.0
- 配置好的eostracker地址为118.190.154.168:4200，可以参考配置