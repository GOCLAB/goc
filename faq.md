* faq是什么意思？
    * faq是frequent asked question的缩写
* 为什么要建立faq.md文档？
    * 由于EOS源码更新较快，其文档更新不及时，且存在纷繁复杂（如docker）的运行方式，经常出现一些卡壳的问题导致跟进者难以继续，因此有必要整理一些实用的工具使用说明、源码解读说明和踩过的坑的说明，以帮助新来者降低学习成本。
* GOC是什么意思？
    * GOC是governance of consensus的缩写，GOC是一条强调治理的公链，GOC的源码是从EOS分支得到，并且会根据需要将EOS最新的改动merge至GOC
* 开发环境
    * GOC项目是C++项目，目前仅在Linux Ubuntu和MacOSX上支持较好，编辑器兼编译器可采用VS Code
* VS Code必备插件

* 代码管理工具
    * GOC的代码在GitHub私有库上，由于项目结构复杂，开发人员众多，建议统一采用sourceTree(下载地址：https://www.sourcetreeapp.com/ 仅支持windows和mac版)进行管理，以便查看其他协作人员（包括EOS人员）提交的代码和注释。
* 下载源码
    * 在新建的GOC的工作目录（以下简写成$GOC）下执行git clone git://github.com/schacon/grit.git
* 编译源码
    * 在$GOC下执行./eosio_build.sh，若出现![avatar](./avatar/build_success.png)表示编译成功
* 常见编译问题
    *
* 编译产物cleos/nodeos/keosd作用与关系
    * 参考说明：https://developers.eos.io/eosio-home/docs/programs-and-tools
    * nodeos: GOC的核心后台进程，nodeos可配置各种插件以作为恰当的节点运行。
    * cleos: GOC的命令行工具，采用REST API的形式与nodeos进行交互,是开发人员常用的命令接口。
    * keosd: GOC的轻量级钱包，用于管理钱包密钥和对交易签名。
    * 三者关系图：[avatar](./avatar/component_relationship.png)
* Eosiocpp工具
    * eosiocpp是将C++转化为wasm和abi的编译器。（wasm：WebAssembly的缩写，wasm的说明可以自行查阅搜索引擎，在GOC中可认为是智能合约对应的可执行文件，abi：Application Binary Interface,程序二进制文件,在EOS中是为了便于合约的接口描述和接口调用）。eosiocpp也即是合约代码的编译器。    

* 智能合约
    * 
* cleos/nodeos/keosd使用说明
    * 由于各命令的子命令较多，纷繁复杂，因此建议阅读$GOC/tutorials/bios-boot-tutorial/bios-boot-tutorial.py文件，快速掌握GOC节点启动、钱包启动、创建公私钥、合约运行等命令
    * 编译后，可执行文件cleos/nodeos/keosd分别在$GOC/build/programs/cleos/ $GOC/build/programs/nodeos/
    $GOC/build/programs/keosd/ 目录下，建议将以上三个目录添加至环境变量
* bios-boot-tutorial.py常用参数
    * ./bios-boot-tutorial.py -k是干掉所有进程
    * ./bios-boot-tutorial.py -w和-W是启动keosd钱
    包
    * ./bios-boot-tutorial.py -b是启动nodeos
    * ./bios-boot-tutorial.py -sctST，做一串工作，分别是生成必要的系统账户，部署系统合约，发SYS币，部署system合约（就是我们主要修改的地方），生成一系列用户账户
    * ./bios-boot-tutorial.py -g是我增加的goc部分数据
    * ./bios-boot-tutorial.py -pPv是生成bp账户，启动bp节点，投票

* GOC创建账号动作分解
    * ./bios-boot-tutorial.py -W 启动钱包时，详见stepStartNewWallet()函数
        * 通过keosd命令（--http-server-address设定钱包服务地址，--wallet-dir设定钱包密钥存放位置）启动钱包
        * cleos wallet create --file xxxx（xxxx文件内容是钱包密码，注意密码与公私钥不是一回事）
        * cleos wallet import --private-key PRIVATEKEY导入公私钥
    * 注：关于cleos/nodeos/keosd的详细子命令可在命令行中查看，常用命令参考./bios-boot-tutorial.py中的用法

* GOC中的表，及CUDR

* GOC中内存、带宽、CPU

* GOC中的抵押、赎回

* GOC中的提案、投票

* GOC中的BP

* GOC中的GN

* GOC源码的plugin机制（待进一步解读）
    * 参考：https://www.cnblogs.com/hbright/p/9234998.html
    * 相互调用与依赖的关系：[avatar](./avatar/plugins.png)
* GOC源码架构（待进一步解读）
    * 视图1：[avatar](./avatar/architecture.png)
    * 视图2：[avatar](./avatar/overview.png)
* systemAccounts有哪些，怎么用？

* 系统合约及作用
    * $GOC/eosio.bios
    * $GOC/eosio.msig
    * $GOC/eosio.system
    * $GOC/eosio.token

* 测试任务及步骤
    * 任务1
        * 步骤。。。