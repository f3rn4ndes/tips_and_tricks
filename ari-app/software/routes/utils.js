const aliases = {
    group1: ['SIP/1002', 'SIP/1003'],
    group2: ['SIP/1004'],
};

function resolveAliases(peers) {
    return peers.flatMap(peer => aliases[peer] || [peer]);
}

function logEvent(eventType, details) {
    console.log(`[${new Date().toISOString()}] ${eventType}:`, details);
}

module.exports = { resolveAliases, logEvent };
