const ari = require('ari-client');
const express = require('express');
const bodyParser = require('body-parser');
const { resolveAliases, logEvent } = require('./routes/utils');
const callRoutes = require('./routes/calls');

const app = express();
app.use(bodyParser.json());

// Load environment variables
require('dotenv').config();

const PORT = process.env.PORT || 3000;
const ARI_URL = process.env.ARI_URL || 'http://192.168.25.166:8088';
const ARI_USERNAME = process.env.ARI_USERNAME || 'sistema';
const ARI_PASSWORD = process.env.ARI_PASSWORD || 'smms1st3m4';

let activeCalls = {};

// Connect to ARI
ari.connect(ARI_URL, ARI_USERNAME, ARI_PASSWORD)
    .then(client => {
        console.log('Connected to ARI!');

        client.start('dynamic_rest_app');

        // Handle ARI events
        client.on('StasisStart', (event, channel) => {
            logEvent('Incoming Call', { event, channel });
            const callID = event.args[0];
            channel.answer().then(() => {
                console.log(`Call ${callID} answered.`);
                channel.hangup();
            });
        });

        // Register REST routes
        callRoutes(app, client, activeCalls);

        // Start the server
        app.listen(PORT, () => console.log(`REST API running on port ${PORT}`));
    })
    .catch(err => console.error('Failed to connect to ARI:', err));

