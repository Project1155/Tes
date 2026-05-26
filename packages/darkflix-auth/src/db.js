'use strict';
/**
 * DarkFlix local session store.
 * Only stores: JWT secrets + current session cache.
 * User profile is stored in Google Drive AppData (cloud-synced).
 */
const fs     = require('fs');
const path   = require('path');
const os     = require('os');
const crypto = require('crypto');

function getDbDir() {
  const appData =
    process.env.APPDATA ||
    (process.platform === 'darwin'
      ? path.join(os.homedir(), 'Library', 'Application Support')
      : path.join(os.homedir(), '.config'));
  const dir = path.join(appData, 'DarkFlix');
  fs.mkdirSync(dir, { recursive: true });
  return dir;
}

const DB_DIR  = process.env.DARKFLIX_DATA_DIR || getDbDir();
const DB_PATH = path.join(DB_DIR, 'auth-db.json');

let _data = null;

function atomicSave() {
  const tmp = DB_PATH + '.tmp';
  fs.writeFileSync(tmp, JSON.stringify(_data, null, 2), 'utf8');
  fs.renameSync(tmp, DB_PATH);
}

async function initDb() {
  try {
    _data = JSON.parse(fs.readFileSync(DB_PATH, 'utf8'));
  } catch {
    _data = {};
  }
  // Auto-generate secrets on first run
  if (!_data.jwtSecret || _data.jwtSecret.length < 64) {
    _data.jwtSecret    = crypto.randomBytes(64).toString('hex');
    _data.cookieSecret = crypto.randomBytes(32).toString('hex');
    console.log('[DarkFlix DB] 🔑 New signing secrets generated');
    atomicSave();
  }
  console.log(`[DarkFlix DB] 📦 Ready at ${DB_PATH}`);
}

function getSecrets() {
  return {
    jwtSecret:    _data.jwtSecret,
    cookieSecret: _data.cookieSecret,
  };
}

module.exports = { initDb, getSecrets, DB_PATH };
