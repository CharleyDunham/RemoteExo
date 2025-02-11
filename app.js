const express = require('express');
const session = require('express-session');
const flash = require('connect-flash');
const path = require('path');

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

// Start the server
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
  console.log(`Server is running on port ${PORT}`);
});
