'use strict';
const router = require('express').Router();
const crypto = require('crypto');
const axios  = require('axios');
const { setSessionCookie } = require('../session');

const PORT     = parseInt(process.env.DARKFLIX_AUTH_PORT || '11471', 10);
const CALLBACK = `http://127.0.0.1:${PORT}/api/google/callback`;

// CSRF state store + auto-purge every 10 min
const pendingStates = new Map();
setInterval(() => {
  const now = Date.now();
  for (const [k, v] of pendingStates) if (v.expires < now) pendingStates.delete(k);
}, 10 * 60 * 1000);

// GET /api/google/config
router.get('/config', (_req, res) => {
  res.json({ configured: !!(process.env.GOOGLE_CLIENT_ID && process.env.GOOGLE_CLIENT_SECRET) });
});

// GET /api/google/start
router.get('/start', (req, res) => {
  const clientId = process.env.GOOGLE_CLIENT_ID;
  if (!clientId || !process.env.GOOGLE_CLIENT_SECRET) {
    return res.status(503).send(`<!DOCTYPE html><html lang="en">
<head><meta charset="UTF-8"><title>Setup Required</title>
<style>body{background:#0d0500;color:#f5dfc0;font-family:sans-serif;display:flex;
align-items:center;justify-content:center;min-height:100vh;margin:0}
.card{background:#1a0c04;border:1px solid #3a1800;border-radius:16px;padding:40px;
max-width:480px;text-align:center}h2{color:#ff6a00;margin-bottom:16px}
code{background:#120802;border:1px solid #3a1800;border-radius:6px;padding:10px 14px;
display:block;text-align:left;font-size:13px;color:#ffbb66;margin:12px 0}
a{color:#ff9933}</style></head>
<body><div class="card">
<h2>⚙️ Google OAuth Setup Required</h2>
<p>Set these environment variables then restart DarkFlix:</p>
<code>GOOGLE_CLIENT_ID=xxx.apps.googleusercontent.com<br>GOOGLE_CLIENT_SECRET=xxx</code>
<p>Get credentials at <a href="https://console.cloud.google.com/">console.cloud.google.com</a></p>
<p style="margin-top:20px"><a href="javascript:history.back()">← Back</a></p>
</div></body></html>`);
  }
  const state = crypto.randomBytes(32).toString('hex');
  pendingStates.set(state, { expires: Date.now() + 10 * 60 * 1000 });
  const params = new URLSearchParams({
    client_id: clientId, redirect_uri: CALLBACK,
    response_type: 'code', scope: 'openid email profile',
    state, prompt: 'select_account', access_type: 'online',
  });
  res.redirect(`https://accounts.google.com/o/oauth2/v2/auth?${params}`);
});

// GET /api/google/callback
router.get('/callback', async (req, res) => {
  const { code, state, error } = req.query;

  if (error) {
    const safe = ['access_denied','interaction_required','login_required'].includes(error)
      ? error : 'cancelled';
    return res.redirect(`/?error=${encodeURIComponent(safe)}`);
  }
  if (!state || typeof state !== 'string') return res.redirect('/?error=invalid_request');

  const pending = pendingStates.get(state);
  if (!pending || pending.expires < Date.now()) {
    pendingStates.delete(state);
    return res.redirect('/?error=session_expired');
  }
  pendingStates.delete(state);
  if (!code || typeof code !== 'string') return res.redirect('/?error=missing_code');

  try {
    const tokenRes = await axios.post(
      'https://oauth2.googleapis.com/token',
      new URLSearchParams({
        code, client_id: process.env.GOOGLE_CLIENT_ID,
        client_secret: process.env.GOOGLE_CLIENT_SECRET,
        redirect_uri: CALLBACK, grant_type: 'authorization_code',
      }),
      { headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, timeout: 10000 }
    );

    const profileRes = await axios.get(
      'https://www.googleapis.com/oauth2/v2/userinfo',
      { headers: { Authorization: `Bearer ${tokenRes.data.access_token}` }, timeout: 10000 }
    );
    const p = profileRes.data;
    if (!p.id) throw new Error('Invalid profile');

    // Embed profile directly in JWT — no DB lookup needed
    setSessionCookie(res, {
      id:          p.id,
      email:       p.email        || '',
      displayName: p.name         || p.email?.split('@')[0] || 'User',
      avatar:      p.picture      || '',
    });
    res.redirect('/?google_success=1');
  } catch (err) {
    console.error('[google/callback]', err.response?.data || err.message);
    res.redirect('/?error=google_auth_failed');
  }
});

module.exports = router;
