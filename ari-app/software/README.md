# Asterisk ARI Telephony App

## Setup
1. Install Node.js and dependencies:


2. Configure Asterisk with PJSIP:
- Edit `config/pjsip.conf`.
- Add endpoints and transports.

3. Start the app:


4. Test REST endpoints:
- **Make a call**: `POST /make-call`
- **List active calls**: `GET /active-calls`
- **Terminate all calls**: `POST /terminate-all-calls`

