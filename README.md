Name
====
ngx_http_jsonnet_module - [Jsonnet](http://jsonnet.org/) for nginx

Synopsis
========
```nginx
http {
  ...
  server {
    location /jsonnet {
      jsonnet on;
    }
  }
}
```

Status
======
Proof of concept.

Installation
============
You'll need libjsonnet and its C API. One way to obtain it is:

```
$ git clone https://github.com/google/jsonnet.git
$ cd jsonnet
$ make libjsonnet.so
```

And then build nginx:

```
$ wget http://nginx.org/download/nginx-1.12.0.tar.gz
$ tar zxf nginx-1.12.0.tar.gz
$ cd nginx-1.12.0
$ LIBJSONNET_LIB=/path/to/jsonnet LIBJSONNET_INC=/path/to/jsonnet/include ./configure --prefix=/tmp/nginx --add-module=/path/to/ngx_http_jsonnet_module && make -j4 && make install
```

Description
===========
This is an nginx module integrating [Jsonnet](http://jsonnet.org/).

```
~ cat /tmp/jsonnet.example
{
    person1: {
        name: "Alice",
        welcome: "Hello " + self.name + "!",
    },
    person2: self.person1 { name: "Bob" },
}

~ curl -d "@/tmp/jsonnet.example2" -X POST http://localhost:9000/jsonnet
{
   "person1": {
      "name": "Alice",
      "welcome": "Hello Alice!"
   },
   "person2": {
      "name": "Bob",
      "welcome": "Hello Bob!"
   }
}
```
