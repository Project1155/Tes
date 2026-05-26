'use strict';
const express      = require('express');
const cors         = require('cors');
const cookieParser = require('cookie-parser');
const path         = require('path');
const { initDb, getSecrets } = require('./db');
const authRouter   = require('./routes/auth');
const googleRouter = require('./routes/google');

const PORT = parseInt(process.env.DARKFLIX_AUTH_PORT || '11471', 10);

async function start() {
  await initDb();
  const { cookieSecret } = getSecrets();
  const app = express();

  // Security headers
  app.use((_req, res, next) => {
    res.setHeader('X-Content-Type-Options', 'nosniff');
    res.setHeader('X-Frame-Options', 'DENY');
    res.setHeader('X-XSS-Protection', '1; mode=block');
    res.setHeader('Referrer-Policy', 'no-referrer');
    res.setHeader('Cache-Control', 'no-store');
    next();
  });

  // CORS: localhost only
  app.use(cors({
    origin: [`http://127.0.0.1:${PORT}`, `http://localhost:${PORT}`],
    credentials: true,
    methods: ['GET', 'POST', 'OPTIONS'],
  }));

  // JSON with error handling → 400 not 500
  app.use((req, res, next) => {
    express.json({ limit: '16kb' })(req, res, err => {
      if (err) return res.status(400).json({ ok: false, error: 'Invalid JSON body.' });
      next();
    });
  });
  app.use(express.urlencoded({ extended: false, limit: '16kb' }));
  app.use(cookieParser(cookieSecret));

  // Static portal (dotfiles blocked)
  app.use(express.static(path.join(__dirname, '..', 'public'), { dotfiles: 'deny' }));

  // Routes
  app.use('/api/auth',   authRouter);
  app.use('/api/google', googleRouter);
  app.get('/api/health', (_req, res) => res.json({ ok: true }));
  app.use('/api', (_req, res) => res.status(404).json({ ok: false, error: 'Not found' }));

  // SPA fallback
  app.get('*', (_req, res) =>
    res.sendFile(path.join(__dirname, '..', 'public', 'index.html')));

  // Global error handler — never leak stack
  // eslint-disable-next-line no-unused-vars
  app.use((err, _req, res, _next) => {
    console.error('[Auth] Error:', err.message);
    res.status(500).json({ ok: false, error: 'Internal server error' });
  });

  app.listen(PORT, '127.0.0.1', () =>
    console.log(`[DarkFlix Auth] ✅ http://127.0.0.1:${PORT}`));
}

start().catch(err => { console.error('[Auth] Fatal:', err.message); process.exit(1); });
