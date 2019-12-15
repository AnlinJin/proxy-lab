# A Simple Concurrent Caching Web Proxy
This web proxy could read and parse different kinds of HTTP request headers from multiple clients concurrently and forward the response from the servers back to the clients. By now, the proxy only accepts GET request. Other request method such as POST will be implemented afterwards.
## Request headers
The web proxy will always send these headers to the server by default 
- User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3)Gecko/20120305 Firefox/10.0.3
- Connection: close
- Proxy-Connection: close

The proxy’s request line will always end with HTTP/1.0.
## Port Numbers
The web proxy listens on a port for upcoming HTTP packets. The port number can be determined by the user. For example 
```
linux> ./proxy 15213
```
The HTTP request port is an optional field in the URL of an HTTP request. That is, the URL may be of the
form, http://www.cmu.edu:8080/hub/index.html, in which case the proxy will connect
to the host www.cmu.edu on port 8080 instead of the default HTTP port, which is port 80.
