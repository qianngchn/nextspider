-- url: the value is the url passed to the lua script by nextspider
-- html: the value is the html content of url passed to the lua script by nextspider
-- nexturl: the value is the next url to access and download for parsing html, will be returned to nextspider
-- files: the value is the files to download, will be returned to nextspider
-- stop: the value determines whether nextspider continues to access url and download files

function parse_html(url, html)
    local nexturl = "https://qianngchn.github.io/index.html";
    local files = "https://qianngchn.github.io/favicon.ico favicon.ico https://qianngchn.github.io/index.html index.html https://qianngchn.github.io/wiki.html wiki.html"
    local stop = true;
    return nexturl, files, stop;
end
