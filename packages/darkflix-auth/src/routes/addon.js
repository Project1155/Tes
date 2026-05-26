'use strict';
/**
 * AIOStreams addon integration.
 * After login, auto-creates a user config in AIOStreams and
 * returns the manifest URL so DarkFlix can install it into Stremio.
 */

const router = require('express').Router();
const crypto = require('crypto');
const axios  = require('axios');
const { db } = require('../db');
const { sessionFromReq } = require('../session');

const AIOSTREAMS_BASE = `http://127.0.0.1:${process.env.AIOSTREAMS_PORT || 11470}`;

function requireAuth(req, res, next) {
  const session = sessionFromReq(req);
  if (!session) return res.status(401).json({ ok: false, error: 'Not authenticated' });
  req.session = session;
  next();
}

// ── GET /api/addon/status ────────────────────────────────────────
// Check if AIOStreams server is running
router.get('/status', async (req, res) => {
  try {
    const r = await axios.get(`${AIOSTREAMS_BASE}/api/v1/health`, { timeout: 2000 });
    res.json({ ok: true, running: true, version: r.data?.version });
  } catch {
    res.json({ ok: true, running: false });
  }
});

// ── POST /api/addon/configure ────────────────────────────────────
// Create/retrieve AIOStreams user config and return manifest URL
router.post('/configure', requireAuth, async (req, res) => {
  try {
    const user = db.findUserById(req.session.id);
    if (!user) return res.status(404).json({ ok: false, error: 'User not found' });

    // If already configured, just return the existing URL
    if (user.aiostreamsUuid) {
      const manifestUrl = `${AIOSTREAMS_BASE}/${user.aiostreamsUuid}/manifest.json`;
      return res.json({ ok: true, manifestUrl, uuid: user.aiostreamsUuid, isNew: false });
    }

    // Try to create a new user config in AIOStreams
    try {
      // AIOStreams creates users automatically when a UUID is hit the first time
      const uuid        = crypto.randomUUID();
      const manifestUrl = `${AIOSTREAMS_BASE}/${uuid}/manifest.json`;

      // Verify the manifest is accessible
      await axios.get(manifestUrl, { timeout: 5000 });

      // Persist the UUID
      db.updateUser(user.id, { aiostreamsUuid: uuid });

      return res.json({ ok: true, manifestUrl, uuid, isNew: true });
    } catch (aioErr) {
      console.warn('[addon/configure] AIOStreams not reachable:', aioErr.message);
      // Return a placeholder manifest URL — addon will work once AIOStreams starts
      const uuid = crypto.randomUUID();
      db.updateUser(user.id, { aiostreamsUuid: uuid });
      const manifestUrl = `${AIOSTREAMS_BASE}/${uuid}/manifest.json`;
      return res.json({ ok: true, manifestUrl, uuid, isNew: true, warning: 'AIOStreams not yet running' });
    }
  } catch (err) {
    console.error('[addon/configure]', err);
    return res.status(500).json({ ok: false, error: 'Failed to configure addon.' });
  }
});

// ── GET /api/addon/manifest-url ──────────────────────────────────
// Quick getter for the authenticated user's manifest URL
router.get('/manifest-url', requireAuth, (req, res) => {
  const user = db.findUserById(req.session.id);
  if (!user) return res.status(404).json({ ok: false, error: 'User not found' });

  if (!user.aiostreamsUuid)
    return res.json({ ok: true, manifestUrl: null, configured: false });

  return res.json({
    ok:          true,
    manifestUrl: `${AIOSTREAMS_BASE}/${user.aiostreamsUuid}/manifest.json`,
    uuid:        user.aiostreamsUuid,
    configured:  true,
  });
});

module.exports = router;
