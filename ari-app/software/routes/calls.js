const { resolveAliases, logEvent } = require('./utils');

module.exports = (app, client, activeCalls) => {
    // Make a Call
    app.post('/make-call', async (req, res) => {
        const { caller, peers, callerID, priority } = req.body;
        if (!caller || !peers || !callerID || typeof priority === 'undefined') {
            return res.status(400).json({ error: 'Missing required fields' });
        }

        const resolvedPeers = resolveAliases(peers);
        const callID = `call_${Date.now()}`;
        console.log(`Initiating call ${callID} with priority ${priority}...`);

        try {
            const successfulPeers = await dialPeers(client, resolvedPeers, callerID, callID);
            activeCalls[callID] = { caller, callerID, peers: resolvedPeers, priority, activePeers: successfulPeers };
            res.json({ callID, status: 'in-progress', activePeers: successfulPeers });
        } catch (err) {
            console.error(`Failed to initiate call ${callID}:`, err.message);
            res.status(500).json({ error: 'Failed to initiate call' });
        }
    });

    // List Active Calls
    app.get('/active-calls', (req, res) => {
        res.json(activeCalls);
    });

    // Terminate All Calls
    app.post('/terminate-all-calls', async (req, res) => {
        const terminatedCalls = [];
        for (const callID of Object.keys(activeCalls)) {
            await terminateCall(client, callID, activeCalls);
            terminatedCalls.push(callID);
        }
        res.json({ message: 'All active calls terminated.', terminatedCalls });
    });
};

async function dialPeers(client, peers, callerID, callID) {
    const successfulPeers = [];
    for (const peer of peers) {
        try {
            const channel = await client.channels.originate({
                endpoint: peer,
                app: 'dynamic_rest_app',
                appArgs: callID,
                callerId: callerID,
            });
            successfulPeers.push(peer);
        } catch (err) {
            console.error(`Failed to dial ${peer}:`, err.message);
        }
    }
    return successfulPeers;
}

async function terminateCall(client, callID, activeCalls) {
    const call = activeCalls[callID];
    if (!call) return;

    console.log(`Terminating call ${callID} with peers: ${call.activePeers}`);
    for (const peer of call.activePeers) {
        const channels = await client.channels.list();
        const channel = channels.find(ch => ch.endpoint === peer);
        if (channel) {
            await client.channels.hangup({ channelId: channel.id });
        }
    }
    delete activeCalls[callID];
}
