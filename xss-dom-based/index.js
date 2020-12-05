// localhost:3000/?username=<script>alert('XSS')</script>
// obf...
// localhost:3000/?username=%3Cscript%3Ealert%28%27XSS%27%29%3C/script%3E
// obf. more...
// localhost:3000/?username=%3C%73%63%72%69%70%74%3Ealert%28%27XSS%27%29%3C/%73%63%72%69%70%74%3E

const express = require('express');
const app = express();
const port = 3000;

app.get('/', (req, res) => {
    res.sendFile('index.html', { root: __dirname });
});

app.listen(port, () => {
    console.log(`App listening at http://localhost:${port}`);
});