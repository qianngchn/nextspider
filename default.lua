-- url: the value is the url passed to the lua script by nextspider
-- html: the value is the html content of url passed to the lua script by nextspider
-- nexturl: the value is the next url to access and download for parsing html, will be returned to nextspider
-- files: the value is the files to download, will be returned to nextspider
-- stop: the value determines whether nextspider continues to access url and download files

function parse_html(url, html)
    local nexturl = "https://github.com/qianngchn/nextspider";
    local files = "https://raw.githubusercontent.com/qianngchn/nextspider/master/config.lua config.lua https://raw.githubusercontent.com/qianngchn/nextspider/master/default.lua default.lua"
    local stop = true;
    print(url);
    print(html);
    return nexturl, files, stop;
end
