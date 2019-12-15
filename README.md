# proxy-lab, A Simple Concurrent Caching Web Proxy
This web proxy based on C could read and parse different kinds of HTTP request headers from multiple clients concurrently and forward the response from the servers back to the clients. By now, the proxy only accepts GET request. Other request method such as POST will be implemented afterwards.
# Request headers
The web proxy will always send these headers by default.
  User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3)Gecko/20120305 Firefox/10.0.3
  Connection: close
  Proxy-Connection: close
The proxy’s request line will always ends with HTTP/1.0.
# parameter
The web proxy takes the port number as its parameter. For example 
linux> ./proxy 15213
