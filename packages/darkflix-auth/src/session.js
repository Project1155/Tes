'use strict';
const jwt = require('jsonwebtoken');
const { getSecrets } = require('./db');

const EXPIRES = '90d';   // 3-month session for desktop app
const COOKIE  = 'darkflix_session';

function signToken(payload) {
  return jwt.sign(payload, getSecrets().jwtSecret, { expiresIn: EXPIRES });
}

function verifyToken(token) {
  try { return jwt.verify(token, getSecrets().jwtSecret); }
  catch { return null; }
}

function setSessionCookie(res, payload) {
  const token = signToken(payload);
  res.cookie(COOKIE, token, {
    httpOnly: true,
    secure:   false,   // localhost only
    sameSite: 'lax',
    maxAge:   90 * 24 * 60 * 60 * 1000,
    path:     '/',
  });
  return token;
}

function clearSessionCookie(res) {
  res.clearCookie(COOKIE, { path: '/' });
}

function sessionFromReq(req) {
  const token =
    req.cookies?.[COOKIE] ||
    (req.headers.authorization?.startsWith('Bearer ')
      ? req.headers.authorization.slice(7)
      : null);
  return token ? verifyToken(token) : null;
}

module.exports = { signToken, verifyToken, setSessionCookie, clearSessionCookie, sessionFromReq, COOKIE };
