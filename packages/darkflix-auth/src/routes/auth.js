'use strict';
/**
 * Auth routes — session check only.
 * Login is via Google OAuth (see google.js).
 * No password registration.
 */
const router = require('express').Router();
const { sessionFromReq, clearSessionCookie } = require('../session');

// ── GET /api/auth/me ──────────────────────────────────────────────
// Returns current user from the JWT (profile is embedded in token)
router.get('/me', (req, res) => {
  const session = sessionFromReq(req);
  if (!session) return res.status(401).json({ ok: false, error: 'Not signed in' });
  return res.json({
    ok:   true,
    user: {
      id:          session.id,
      email:       session.email,
      displayName: session.displayName,
      avatar:      session.avatar,
      provider:    'google',
    },
  });
});

// ── GET /api/auth/status ──────────────────────────────────────────
// Lightweight check used by C++ shell on startup
router.get('/status', (req, res) => {
  const session = sessionFromReq(req);
  if (!session) return res.json({ authenticated: false });
  return res.json({
    authenticated: true,
    userId:        session.id,
    displayName:   session.displayName,
    email:         session.email,
    avatar:        session.avatar,
  });
});

// ── POST /api/auth/logout ─────────────────────────────────────────
router.post('/logout', (req, res) => {
  clearSessionCookie(res);
  res.json({ ok: true });
});

module.exports = router;
