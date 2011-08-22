/*
 * Last Change: 13-Jan-2003.
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
# include <winsock2.h>
# pragma comment(lib, "ws2_32")
#endif

#include "base64.h"
#include "etclib.h"
#include "http.h"

#define LOG_MSG(x) printf x
#define LOG(m, x) LOG_##m (x)

#define HTTP_SESSION_BUFSIZE 4096

struct _http_t
{
    char    *useragent;
    int	    mode_get;
    char    *proxy;
    int	    proxy_port;
    char    *proxyauth;
    int	    cache;

    HTTP_GET_PROGRESS	getprogress_proc;
    void*		getprogress_value;
};

    void
http_set_cache(http_t* http, int state)
{
    /* state == 0の時キャッシュ無効 */
    http->cache = state;
}

    void
http_set_proxy(http_t* http, const char* proxy, const char* auth)
{
    char *sep;

    /* 旧設定削除 */
    free(http->proxy);
    free(http->proxyauth);

    http->proxy = proxy ? strdup(proxy) : NULL;
    /* プロキシポート読み込み */
    http->proxy_port = 8080;
    if (sep = strchr(http->proxy, ':'))
    {
	*sep = '\0';
	if (isdigit(sep[1]))
	    http->proxy_port = atoi(&sep[1]);
    }
    /* プロキシ認証設定 */
    http->proxyauth = auth ? strdup(auth) : NULL;
}

    void
http_set_proc_getprogress(http_t* http, HTTP_GET_PROGRESS proc, void* value)
{
    /*
     * GET動作のプログレスバーを実装するためのコールバック関数を設定する。
     * コールバック関数は通常1(非零)を返す必要がある。GETを終了したい場合に
     * はコールバックが0を返せば良い。
     */
    if (http)
    {
	http->getprogress_proc = proc;
	http->getprogress_value = value;
    }
}

    http_t*
http_open()
{
    http_t *http = (http_t*)calloc(1, sizeof(http_t));

    if (http)
    {
	http->cache = 1;
    }
    return http;
}

    void
http_close(http_t* http)
{
    if (http)
    {
	free(http->useragent);
	free(http->proxy);
    }
    free(http);
}

    void
http_set_useragent(http_t *http, const char* useragent)
{
    if (http->useragent)
	free(http->useragent);
    http->useragent = useragent ? strdup(useragent) : NULL;
}

#ifndef ETCLIB_H
    static char*
strndup(char* src, int len)
{
    char *p = NULL;

    if (src && len >= 0 && (p = malloc(len + 1)))
    {
	memcpy(p, src, len);
	p[len] = '\0';
    }
    return p;
}
#endif

#define HTTP_MODEGET_HEAD 0
#define HTTP_MODEGET_NOTHEAD 1
#define HTTP_MODEGET_WAITLF 2
#define HTTP_MODEGET_WAITLF_PAYLOAD 3
#define HTTP_MODEGET_PAYLOAD 4

    static int
parse_recv(http_t* http, char** /*OUT*/ start, char* buf, int len)
{
    int i;

    for (i = 0; i < len; ++i)
    {
	if (http->mode_get == HTTP_MODEGET_PAYLOAD)
	    break;
	switch (http->mode_get)
	{
	    case HTTP_MODEGET_HEAD:
		switch (buf[i])
		{
		    case '\r':
			http->mode_get = HTTP_MODEGET_WAITLF_PAYLOAD;
			break;
		    case '\n':
			http->mode_get = HTTP_MODEGET_PAYLOAD;
			break;
		    default:
			http->mode_get = HTTP_MODEGET_NOTHEAD;
			LOG(MSG, ("  %c", buf[i]));
			break;
		}
		break;
	    case HTTP_MODEGET_NOTHEAD:
		switch (buf[i])
		{
		    case '\r':
			http->mode_get = HTTP_MODEGET_WAITLF;
			break;
		    case '\n':
			http->mode_get = HTTP_MODEGET_HEAD;
			LOG(MSG, ("\n"));
			break;
		    default:
			LOG(MSG, ("%c", buf[i]));
		}
		break;
	    case HTTP_MODEGET_WAITLF:
		if (buf[i] == '\n')
		{
		    http->mode_get = HTTP_MODEGET_HEAD;
		    LOG(MSG, ("\n"));
		}
		break;
	    case HTTP_MODEGET_WAITLF_PAYLOAD:
		if (buf[i] == '\n')
		    http->mode_get = HTTP_MODEGET_PAYLOAD;
		break;
	    case HTTP_MODEGET_PAYLOAD:
		break;
	}
    }

    if (start)
	*start = buf + i;
    return len - i;
}

    static int
get_session(http_t* http, char* url, char* host, int port, char* remote,
	char* local)
{
    FILE *fp = NULL;
    SOCKET s = INVALID_SOCKET;
    SOCKADDR_IN addr_in;
    unsigned long remote_address;
    char buf[HTTP_SESSION_BUFSIZE];
    int size = 0;

    LOG(MSG, ("get_session()\n"));

    /* 必要なリソースの収集 */
    {
	char* truehost = http->proxy ? http->proxy : host;
	HOSTENT *entry;

	if (entry = gethostbyname(truehost))
	    remote_address = *((unsigned long*)entry->h_addr);
	else
	{
	    LOG(MSG, ("  gethostbyname(%s) failed\n", truehost));
	    if ((remote_address = inet_addr(truehost)) == INADDR_NONE)
	    {
		LOG(MSG, ("  inet_addr(%s) failed\n", truehost));
		goto SESSION_ERROR;
	    }
	}
    }
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	goto SESSION_ERROR;
    /* プロキシ使用時にはURLをリモートパスにしてプロキシポート番号を使用 */
    if (http->proxy)
    {
	remote = url;
	port = http->proxy_port;
    }

    /* ホストへ接続 */
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons((u_short)(port < 1 ? HTTP_PORT : port));
    addr_in.sin_addr.s_addr = remote_address;
    if (connect(s, (SOCKADDR*)&addr_in, sizeof(addr_in)) == SOCKET_ERROR)
	goto SESSION_ERROR;

    /* ヘッダ送信 */
    send(s, "GET ", 4, 0);
    send(s, remote, strlen(remote), 0);
    send(s, " HTTP/1.0", 9, 0);
    send(s, "\r\n", 2, 0);
    /* ユーザエージェント設定 */
    if (http->useragent)
    {
	send(s, "User-Agent: ", 12, 0);
	send(s, http->useragent, strlen(http->useragent), 0);
	send(s, "\r\n", 2, 0);
    }
    /* 想定ホスト */
    send(s, "Host: ", 6, 0);
    send(s, host, strlen(host), 0);
    send(s, "\r\n", 2, 0);
    /* キャッシュ: when state == 0, disable */
    if (http->cache == 0)
    {
	send(s, "Pragma: no-cache", 16, 0);
	send(s, "\r\n", 2, 0);
    }
    /* プロキシ認証 */
    if (http->proxyauth)
    {
	char *enc = base64_encode(http->proxyauth);

	if (enc)
	{
	    send(s, "Proxy-Authorization: Basic ", 27, 0);
	    send(s, enc, strlen(enc), 0);
	    send(s, "\r\n", 2, 0);
	    free(enc);
	}
    }
    /* 最後に空行を送る */
    send(s, "\r\n", 2, 0);

    /* 受信・保存 */
    fp = fopen(local, "wb");
    http->mode_get = HTTP_MODEGET_HEAD;
    while (1)
    {
	int len_recv;

	len_recv = recv(s, buf, sizeof(buf), 0);
	if (len_recv == 0 || len_recv == SOCKET_ERROR)
	    break;
	else
	{
	    int len;
	    char *start;

	    /*LOG(MSG, ("  len_recv()=%d\n", len_recv));*/
	    len = parse_recv(http, &start, buf, len_recv);
	    if (fp && start && len > 0)
		fwrite(start, 1, len, fp);
	    size += len;
	    /* プログレスバーの検証 */
	    if (http->getprogress_proc
		    && http->getprogress_proc(http->getprogress_value,
			    size) == 0)
	    {
		size = HTTP_GETERROR_TERMINATED;
		break;
	    }
	}
    }

    closesocket(s);
    if (fp)
	fclose(fp);
    return size;

SESSION_ERROR:
    if (fp)
	fclose(fp);
    if (s != INVALID_SOCKET)
	closesocket(s);
    return HTTP_GETERROR;
}

/* エラー時には負の数を返す */
    int
http_get(http_t* http, char* filename, char* url)
{
    char *last;
    char *host, *remote_path;
    char *local_path = NULL;
    int port = 80;
    int result = 0;

    LOG(MSG, ("http_get()\n"));

    if (strncmp(url, "http://", 7))
	return HTTP_GETERROR;

    /* URLからホスト名とリモートパスを分離する。 */
    if (last = strchr(&url[7], '/'))
    {
	int len = last - &url[7];

	if (len <= 0)
	    return HTTP_GETERROR;
	host = strndup(&url[7], len);
	remote_path = strdup(last);
    }
    else
    {
	if (strlen(&url[7]) <= 0)
	    return HTTP_GETERROR;
	host = strdup(&url[7]);
	remote_path = strdup("/");
    }

    /* ホスト名からポート番号を切り出す。 */
    if ((last = strchr(host, ':')) && isdigit(last[1]))
    {
	*last = '\0';
	port = atoi(last + 1);
    }

    /* ファイル名が指定されない場合に自分で生成する。 */
    if (!filename)
    {
	int len = strlen(remote_path);

	if (remote_path[len - 1] == '/')
	    local_path = strdup("index");
	else
	{
	    if (last = strrchr(remote_path, '/'))
		local_path = strdup(last);
	    else
		local_path = strdup(remote_path);
	}
	filename = local_path;
    }

    LOG(MSG, ("  host=%s\n", host));
    LOG(MSG, ("  port=%d\n", port));
    LOG(MSG, ("  remote_path=%s\n", remote_path));
    LOG(MSG, ("  local_path=%s\n", filename));
    /*
     * PROXY使用時に:
     * URLがディレクトリを指している時には取れないかもしれない?  →末尾に
     * 「/」を追加する必要がある。でも…URLだけからそれがディレクトリなのか
     * どうなのかという判断はつかない。
     */
    result = get_session(http, url, host, port, remote_path, filename);

    free(host);
    free(remote_path);
    free(local_path);
    return result;
}

/*
 * 以下、テスト用main()
 */
#if !defined(_USRDLL) && !defined(_LIB)
    static int
download(char* url, char* localfile, char* proxy)
{
    int result = -1;
    http_t *http;

    printf("download(url=%s, localfile=%s, proxy=%s)\n", url, localfile, proxy);
    if (url && (http = http_open()))
    {
	http_set_useragent(http, "http/0.0 (HTTP client sample)");
	if (proxy)
	    http_set_proxy(http, proxy, NULL);
	result = http_get(http, localfile ? localfile : "http.log", url);
	LOG(MSG, ("result=%d\n", result));
	http_close(http);
    }

    return result;
}

    int
main(int argc, char** argv)
{
    char *url = NULL, *localfile = NULL, *proxy = NULL;
#ifdef _WIN32
    {
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(1, 1), &wsa))
	    return -1;
    }
#endif

    for (++argv; *argv; ++argv)
    {
	if (!strcmp(*argv, "-o") || !strcmp(*argv, "--output"))
	{
	    if (argv[1])
		localfile = *++argv;
	}
	else if (!strcmp(*argv, "-p") || !strcmp(*argv, "--proxy"))
	{
	    if (argv[1])
		proxy = *++argv;
	}
	else
	{
	    if (!url)
		url = *argv;
	    else if (!localfile)
		localfile = *argv;
	    else if (!proxy)
		proxy = *argv;
	    else
		break;
	}
    }
    download(url, localfile, proxy);

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
#endif /* !defined(_USRDLL) && !defined(_LIB) */
