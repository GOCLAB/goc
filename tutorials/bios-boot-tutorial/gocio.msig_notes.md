#gocio.msig分析

##相关命令
可以通过cleos get account <name>来查看当前帐号的permissions信息，如：
```sh
cleos get account useraaaaaaaa

permissions: 
     owner     1:    1 GOC69X3383RzBZj41k73CSjUNXM5MYGpnDxyPnWUKPEtYQmTBWz4D
        active     1:    1 GOC69X3383RzBZj41k73CSjUNXM5MYGpnDxyPnWUKPEtYQmTBWz4D
```
permissions 默认有两种，分别是owner和active，其中owner权限比active更大。以上的信息是指要执行owner的相关操作要求的阈值是1，公钥 GOC69X33...的权重是1。active类似。

##相关table
* 有两个重要的表，分别是proposal，approvals_info。
  * proposal主键是proposal_name（提案名），保存该提案的交易信息。
  * approvals_info主键是proposal_name（提案名），保存通过该提案所需的权限和已经审批通过的信息。approve()和unapprove()只修改这个表的信息。

##相关代码
该合约有以下action:
```cpp
propose()
approve( account_name proposer, name proposal_name, permission_level level )
unapprove( account_name proposer, name proposal_name, permission_level level )
cancel( account_name proposer, name proposal_name, account_name canceler )
exec( account_name proposer, name proposal_name, account_name executer )
```
对应cli 命令是cleos multisig SUBCOMMAND，SUBCOMMAND 是上面的action名。

* **propose**()是创建一个提案。
  * 参数包括提案发起帐户，提案名，提案通过所需权限，提案所要执行的具体内容等信息，
  * 参数通过read_action_date()获取。会调用check_transaction_authorization()检查提案所要执行的内容是否合法。
  * 将提案相关内容写入两个表proptable(proposal table)，apptable(approvals_info table)。

* **approve**() 是通过提案，参数分别是
   * proposer 提案的发起帐户
   * proposal_name 提案名
   * level 使用哪个权限批准这个提案
  
* **unapprove**()是不通过提案，参数同上
  
* **cancel**() 撤消提案
  * proposer 提案的发起帐户
  * proposal_name 提案名
  * canceler 取消提案帐户，在提案未超期之前（默认24h），必须与提案发起帐户一致，

* **exec**() 执行提案
  * proposer 提案的发起帐户
  * proposal_name 提案名
  * executer 执行帐户
  * 通过check_transaction_authorization()判断同意的权重总和是否达到阈值。如果交易执行权限检验无误，会发起一个defer延迟合约，去执行提案交易

##与GOC proposal的区别
msig中的propose 是指一系列action的集合，可以认为是一系列要执行的动作。GOC新增的proposal是指一个提议，对GOC社区改进的建议，类似于比特币中的BIP。


[**参考**]
https://www.cnblogs.com/Evsward/p/msig.html
https://www.jianshu.com/p/95d3fea628df