const express = require('express');
const session = require('express-session');
const flash = require('connect-flash');
const path = require('path');
const http = require('http');
const WebSocket = require('ws'); // Raw WebSocket library

const app = express();

// In-memory shared state (server state)
const sharedState = {};

// Set EJS as the templating engine
app.set('view engine', 'ejs');
app.set('views', path.join(__dirname, 'views'));

// Middleware for parsing URL-encoded bodies (form data) and JSON
app.use(express.urlencoded({ extended: true }));
app.use(express.json());

// Setup sessions and flash messaging
app.use(session({
  secret: 'your-secret-key', // Replace with a secure secret in production
  resave: false,
  saveUninitialized: true
}));
app.use(flash());

// Make flash messages available to all views
app.use((req, res, next) => {
  res.locals.flash = req.flash();
  next();
});

/**
 * Routes:
 */

// Render the index page with forms
app.get('/', (req, res) => {
  res.render('index');
});

// POST /set_value: Save a generic key/value pair into sharedState
app.post('/set_value', (req, res) => {
  const key = req.body.key || "default_key";
  const value = req.body.value;
  sharedState[key] = value;

  req.flash('notice', `Value set successfully: ${key} = ${value}`);
  res.redirect('/');
});

// GET /get_value: Retrieve a value by key (returns JSON)
app.get('/get_value', (req, res) => {
  const key = req.query.key || "default_key";
  const value = sharedState[key];

  if (value !== undefined) {
    res.json({ key, value });
  } else {
    res.status(404).json({ error: `Key not found: ${key}` });
  }
});

// POST /set_flexion: Set the flexion state in sharedState
app.post('/set_flexion', (req, res) => {
  const flexion_state = req.body.flexion || "0";
  sharedState["flexion"] = flexion_state;

  req.flash('notice', `Flexion state set to: ${flexion_state}`);
  res.redirect('/');
});

// GET /get_flexion: Get the flexion state (returns plain text)
app.get('/get_flexion', (req, res) => {
  const value = sharedState["flexion"];

  if (value !== undefined) {
    res.type('text').send(value);
  } else {
    res.status(404).send("Flexion value not set");
  }
});

// Create an HTTP server using the Express app
const server = http.createServer(app);

// Create a WebSocket server that attaches to the HTTP server
const wss = new WebSocket.Server({ server });

wss.on('connection', (ws) => {
  console.log('A client connected via raw WebSocket');

  ws.on('message', (message) => {
    // Convert the message buffer to a string
    const msgString = message.toString();
    console.log('Received via raw WebSocket:', msgString);
    // Echo the message back to the client
    ws.send('Command received: ' + msgString);
  });

  ws.on('close', () => {
    console.log('Client disconnected from raw WebSocket');
  });
});

// Start the server using the HTTP server (with WebSocket attached)
const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
  console.log(`Server is running on port ${PORT}`);
});
