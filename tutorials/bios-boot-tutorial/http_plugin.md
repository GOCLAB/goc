* plugin生命周期请参考 [faq.md](../../faq.md)

* http_plugin注册(register_plugin)之后，使用方法如下(以keosd为例):
 ```cpp
    auto& http = app().get_plugin<http_plugin>();
    http.add_handler("/v1/keosd/stop", [](string, string, url_response_callback cb) { cb(200, "{}"); std::raise(SIGTERM); } );
```    
* http_plugin::add_handler函数
    * add_handler(const string& url, const url_handler& handler)，通常是在http_plugin注册之后调用
    * add_handler将上述两个参数url，handler放到一个url_handlers的map数据结构中
    * url_handlers会在handle_http_request中时调用，而handle_http_request在插件启动中调用，
    * 可以直接调用，也可将多个add_handler封装在add_api中调用（如wallet_api_plugin中）
    * 问题：add_handler能在插件启动后调用了吗？
    
* http_plugin::set_program_options
    * 对http服务器进行配置，比如：keosd启动默认打开127.0.0.1:8888访问地址。
    
* http_plugin::plugin_initialize流程
    1. 解析ip地址和端口，并对该ip和端口建立ipv4的结构体封装
    2. 如有需要支持https，则需要指定证书链文件、私钥文件，并对该ip和端口建立ipv4的结构体封装

* http_plugin::plugin_startup流程
    1. 如果http_plugin::plugin_initialize的结构体封装成功，则监听该套接字。
    ```cpp
    my->create_server_for_endpoint(*my->listen_endpoint, my->server);
    ```
    2. 在create_server_for_endpoint中调用handle_http_request设置每一次http请求的处理逻辑，handle_http_request中通过url_handlers这样一个map<url,handler>完成url:handler的维护
    3. handle_http_request中还检测和添加一定的http头部
    4. 最后通过如下代码完成http handler的回调处理。
    ```cpp
        con->defer_http_response();
        handler_itr->second( resource, body, [con]( auto code, auto&& body ) {
            con->set_body( std::move( body ));
            con->set_status( websocketpp::http::status_code::value( code ));
            con->send_http_response();
        } );
        ```
* 具体使用（仍然以上述keosd的代码为例）
    * 当用户启动keosd后，在另一个命令行调用curl http://127.0.0.1:8888/v1/keosd/stop 将杀死刚刚启动的keosd进程，分析如下：
    * 由于在http_plugin注册时调用了，设置了/v1/keosd/stop的处理函数，即[](string, string, url_response_callback cb) { cb(200, "{}"); std::raise(SIGTERM); }，并将这样一个对应关系记录在了url_handlers的map中
    * 在plugin_initialize到plugin_startup过程中开启了127.0.0.1:8888服务器，并且设置了所有对该服务请求的处理方式
    * 处理方式即是，根据http请求的路径在url_handlers中找到对应的回调函数
    * 当请求链接http://127.0.0.1:8888/v1/keosd/stop 时，url_response_callback被执行，http返回码是200，返回内容是"{}"，并且向系统发出SIGTERM信号，使得keosd进程退出。