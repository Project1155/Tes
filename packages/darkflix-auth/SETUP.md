# DarkFlix Auth — Setup Guide

## هل تحتاج قاعدة بيانات أو سيرفر خارجي؟

**الجواب القصير: لا ✅**

DarkFlix تطبيق سطح مكتب يعمل محلياً.  
كل بيانات المستخدمين تُخزَّن في:

```
Windows: %APPDATA%\DarkFlix\auth-db.json
```

- لا سيرفر خارجي مطلوب
- لا اشتراك مدفوع
- يعمل offline بالكامل
- الملف مشفر بـ JWT (secret مُولَّد تلقائياً)

---

## إعداد Google OAuth (اختياري)

### الخطوة 1: إنشاء مشروع Google

1. افتح https://console.cloud.google.com/
2. اضغط **New Project** → أدخل اسم "DarkFlix"
3. من القائمة الجانبية: **APIs & Services** → **Credentials**

### الخطوة 2: إنشاء OAuth Client

1. اضغط **Create Credentials** → **OAuth 2.0 Client ID**
2. Application type: **Web application**
3. Name: `DarkFlix Desktop`
4. Authorized redirect URIs → أضف:
   ```
   http://127.0.0.1:11471/api/google/callback
   ```
5. اضغط **Create**
6. ستظهر نافذة بـ **Client ID** و **Client Secret** — احتفظ بهما

### الخطوة 3: تفعيل Google API

1. من القائمة: **APIs & Services** → **Library**
2. ابحث عن **Google+ API** أو **Google Identity** → Enable

### الخطوة 4: إضافة المتغيرات

**طريقة 1 — Windows Environment Variables (مستحسن):**
```powershell
# في PowerShell كـ Administrator
[System.Environment]::SetEnvironmentVariable("GOOGLE_CLIENT_ID","your-client-id.apps.googleusercontent.com","Machine")
[System.Environment]::SetEnvironmentVariable("GOOGLE_CLIENT_SECRET","your-secret","Machine")
```

**طريقة 2 — ملف .env (للتطوير):**  
أنشئ ملف `packages/darkflix-auth/.env`:
```env
GOOGLE_CLIENT_ID=your-client-id.apps.googleusercontent.com
GOOGLE_CLIENT_SECRET=your-secret
```

**طريقة 3 — في GitHub Actions Secrets:**
```
Settings → Secrets → Actions → New secret
GOOGLE_CLIENT_ID = your-client-id
GOOGLE_CLIENT_SECRET = your-secret
```

### الخطوة 5: تشغيل DarkFlix

أعد تشغيل التطبيق — سيظهر زر **"Continue with Google"** تلقائياً ✅

---

## الـ Addons المُضمَّنة (تُثبَّت تلقائياً بعد تسجيل الدخول)

| Addon | الوصف | الرابط |
|-------|-------|--------|
| **OpenSubtitles Pro** | ترجمة عربية تلقائية | opensubtitlesv3-pro.dexter21767.com |
| **Torrentio** | مصادر تورنت، مرتبة بالجودة، عربي | torrentio.strem.fun |

---

## خيارات Cloud (إذا أردت مزامنة البيانات لاحقاً)

| الخيار | السعر | التقييم |
|--------|-------|---------|
| **Supabase** | مجاني (500MB) | ⭐⭐⭐⭐⭐ — أفضل خيار |
| **Firebase** | مجاني (1GB) | ⭐⭐⭐⭐ |
| **PocketBase** | مجاني (self-host) | ⭐⭐⭐⭐ |

**حالياً غير مطلوب** — الـ JSON local storage كافٍ تماماً لتطبيق سطح المكتب.

---

## ملخص الثغرات المُصلَّحة

| # | الثغرة | الحل |
|---|--------|------|
| 1 | JWT secret مضمَّن في الكود | يُولَّد تلقائياً عند التشغيل الأول |
| 2 | لا rate limiting | 20 req/15min على login، 10 req/15min على register |
| 3 | CORS يقبل أي origin | مقيَّد بـ `127.0.0.1:11471` فقط |
| 4 | رسائل خطأ Google تكشف تفاصيل داخلية | sanitized إلى رسائل عامة |
| 5 | WinHTTP بلا حد أقصى للـ body | cap بـ 4KB |
| 6 | pendingStates لا تُنظَّف | cleanup كل 10 دقائق |
| 7 | displayName فارغ بعد sanitize | fallback للـ username |
| 8 | port 11471 مكرر في 5+ أماكن | ثابت `DARKFLIX_AUTH_PORT` |
| 9 | env var خاص بـ AIOStreams (محذوف) | محذوف |
| 10 | تعليق token-in-URL قديم | محذوف |
| 11 | error redirect يكشف رسالة داخلية | whitelist أسباب محددة فقط |
| 12 | cookie secret = JWT secret | سرّان مختلفان من الـ DB |
| 13 | pendingStates تتراكم في الذاكرة | setInterval لتنظيفها |
