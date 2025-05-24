#!/bin/bash
# filepath: /home/noworld/test/MyPoorWebServer/cpp/httpdocs/post.cgi

# 输出HTTP头
echo "Content-type: text/html"
echo ""

# 打印HTML开始部分
cat << HTML_START
<html>
<head>
    <title>POST Data</title>
    <meta charset="utf-8">
</head>
<body>
    <h2>Your POST data:</h2>
HTML_START

# 如果是POST请求，处理POST数据
if [ "$REQUEST_METHOD" = "POST" ]; then
    # 获取内容长度
    if [ -n "$CONTENT_LENGTH" ]; then
        # 读取POST数据
        POST_DATA=$(dd bs=1 count=$CONTENT_LENGTH 2>/dev/null)
        
        # 显示所有POST数据
        echo "<ul>"
        echo "$POST_DATA" | tr '&' '\n' | while read param; do
            echo "<li>$param</li>"
        done
        echo "</ul>"
    else
        echo "<p>No content length provided</p>"
    fi
# 如果是GET请求，显示查询字符串
elif [ "$REQUEST_METHOD" = "GET" ]; then
    if [ -n "$QUERY_STRING" ]; then
        echo "<ul>"
        echo "$QUERY_STRING" | tr '&' '\n' | while read param; do
            echo "<li>$param</li>"
        done
        echo "</ul>"
    else
        echo "<p>No query string provided</p>"
    fi
else
    echo "<p>Unknown request method: $REQUEST_METHOD</p>"
fi

# 显示环境信息
echo "<h3>Environment Information:</h3>"
echo "<ul>"
echo "<li>REQUEST_METHOD: $REQUEST_METHOD</li>"
echo "<li>QUERY_STRING: $QUERY_STRING</li>"
echo "<li>CONTENT_LENGTH: $CONTENT_LENGTH</li>"
echo "<li>SCRIPT_NAME: $SCRIPT_NAME</li>"
echo "<li>SERVER_NAME: $SERVER_NAME</li>"
echo "<li>SERVER_PORT: $SERVER_PORT</li>"
echo "</ul>"

# HTML结束
cat << HTML_END
</body>
</html>
HTML_END


