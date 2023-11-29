https://greenbytes.de/tech/webdav/rfc7230.html#http.message

### Charset
- parameter inside "Content-Type" header
- implementors MUST use the charset specified in the request header IF it represents a charset defined by IANA;

### Content Codings
- headers like "Content-Encoding" and "Accept-Encoding";
- expresses how data is encoded, allowing for compression of documents sent, etc;
- as of 1.1, valid values are "gzip", "compress", "deflate", "identity";

### Intermediaries
- three types: proxy, gateway and tunnel;
- proxy: forwards messages, may translate, apply transformations or even satisfy the received messages - usually used to group a organization's traffic for security, logging and caching reasons;
- gateweay: a.k.a. "reverse proxy", acts as an origin server for the requests, but forwards it "inbound" to other serves;
- tunnel: blindly relays the received message - no translation or changes are done;

### Message Format
HTTP-message =	start-line
				*( header-field CRLF)
				CRLF
				[ message-body ]

- parsing = read start-line (struct), read headers (hash table), determine if there is body and, if there is, read until body length is reached or connection is closed;
- should reject messages with whitespaces before first header or ignore those lines altogether;
- when parsing header fields, should reject requests with whitespace between field name and following colon;
- when parsing header values, should reject requests with CRLF (obs-fold) before any text;
