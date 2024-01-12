# AuthManager

Just a webhook tool

## Features

Monitor the configuration file, hot restart (but the restart will fail if the configuration file format is wrong)

## File & Dir
`rules.json` : 

> Define the rules of config, All RouterConifgs should contain the corresponding rule, no need to modify usually.

`AuthServerStartup` : 

> the contorlor of program, you can execute as `./AuthServerStartup.sh start/stop/status/clean`.

`res/` :

> you can put your html or program here for router using.

`RouterConfigs/` : 

> All routerconfigs should put in this directory with suffix `.json`, so that program can read of them.

`bin/config/` : 

> save basic config of http server, it will be generated when program first startup

`bin/log/` : 

> save log of httpserver

## Startup

### Envirnment
#### C++
C++ version >= 17
```
apt-get update
apt-get install -y gcc g++
```

#### Xmake
Document: [https://xmake.io/#/](https://xmake.io/#/)
```bash
wget https://xmake.io/shget.text -O - | bash
```


### Compile (project root path)
```bash
xmake
```

## Usage
### Script `AuthServerStartup`
```bash
Usage:  $ ./AuthServerStartup [options]
optoins:
    start       Start the AuthManager and monitor  
    stop        Stop AuthManager
    status      Show process status of AuthManager
    clean       Remove all log in bin/logs

```

### Configs in `RouterConfigs`
> all configs file should be set in form of JSON

Example:
```json
{
    "default" : {
        "router" : "/hello",
        "type" : "text",
        "text" : "hello world",
        "discription" : "show text 'hello world'"
    },
    "hello" : {
        "router" : "/hello",
        "type" : "html",
        "path" : "/opt/www/hello.html",
        "discription" : "show hello page"
    },
    "ls" : {
        "router" : "/ls",
        "type" : "program",
        "command" : "ls -al",
        "discription" : "show command result"
    }
}
```
Key `"default" "hello" "ls"` has no function, you can use any name as your like

You can find the support type in `rules.json`

Such as type `"text"`

In rule file:
```json
{
    "name": "TEXT",
    "type":"text",
    "syntax": {
        "router": "string",
        "type": "string",
        "text": "string"
    }
},
```
it syntax has three option `"router" "type" "text"`
so all of three option should be set in config
```json
"default" : {
        "router" : "/hello",
        "type" : "text",
        "text" : "hello world",
        "discription" : "show text 'hello world'"
    },
```
`"discription"` is not necessary,you can remove or change it.