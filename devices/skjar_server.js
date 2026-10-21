// Serves skjar_player.html on localhost, on whatever port is free, and tells
// Max which one. jweb~ then loads the page over http, so it has an origin —
// the YouTube embed will not play in a page read from disk.
const maxApi = require("max-api");
const http = require("http");
const fs = require("fs");
const path = require("path");

const page = path.join(__dirname, "skjar_player.html");

const server = http.createServer((req, res) => {
    if (req.url !== "/" && ! req.url.startsWith("/?")) {
        res.writeHead(404);
        res.end();
        return;
    }
    fs.readFile(page, (err, data) => {
        if (err) {
            res.writeHead(500);
            res.end(String(err));
            return;
        }
        res.writeHead(200, { "Content-Type": "text/html; charset=utf-8", "Cache-Control": "no-store" });
        res.end(data);
    });
});

server.listen(0, "127.0.0.1", () => maxApi.outlet("port", server.address().port));
