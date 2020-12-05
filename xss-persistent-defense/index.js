// <script>document.getElementsByTagName('body')[0].style.backgroundColor = 'red';</script>
// XSS highlighted post<script id="xss">document.getElementById('xss').parentNode.style.backgroundColor='yellow';</script>

const express = require('express');
const app = express();
const port = 3000;

page1 = '<h1>XSS Vulnerable Site</h1><h3>Make a post</h3><textarea name="content" form="form"></textarea><form action="/makepost" method="get" id="form"><input type="submit" value="Submit"/></form><br/><h3>Posts</h3><table>'
page2 = '</table>';

let posts = [];

app.get('/', (req, res) => {
    let index = page1;
    for (var post of posts)
    {
        index += '<tr><td>';
        index += post;
        index += '</td></tr>';
    }
    index += page2;
    res.send(index);
});

app.get('/makepost', (req, res) => {
    console.log(req.query)
    var post = req.query.content;
    post = post.replace(/</gi, '&lt;').replace(/>/gi, '&gt;');
    posts.push(post);
    res.redirect('/');
});

app.listen(port, () => {
    console.log(`App listening at http://localhost:${port}`);
});