require('dotenv').config(); // Load environment variables from .env
const net = require('net');
const axios = require('axios');

// Load environment variables
const SERVER_PORT = parseInt(process.env.SERVER_PORT, 10) || 1023;
const TIMEOUT_DURATION = parseInt(process.env.TIMEOUT_DURATION, 10) || 3000;
const PBX_ADDRESS = process.env.PBX_ADDRESS;
const PBX_ARI_PORT = process.env.PBX_ARI_PORT;
const PBX_USER = process.env.PBX_USER;
const PBX_PASSWORD = process.env.PBX_PASSWORD;
const PBX_ENDPOINT = process.env.PBX_ENDPOINT;
const PBX_EXTENSION = process.env.PBX_EXTENSION;
const PBX_CONTEXT = process.env.PBX_CONTEXT;
const PBX_PRIORITY = parseInt(process.env.PBX_PRIORITY, 10) || 1;
const PBX_CHANNEL_ID = process.env.PBX_CHANNEL_ID;
const PBX_MAX_CALL_TIME = process.env.PBX_MAX_CALL_TIME;

// Buffer to store received PDUs
let messageBuffer = [];
let timer = null;

// Substitution rules for encoding and decoding
const substitutionRules = {
    encode: {
        0x02: [0x1B, 0x82],
        0x03: [0x1B, 0x83],
        0x06: [0x1B, 0x86],
        0x15: [0x1B, 0x95],
        0x1B: [0x1B, 0x9B],
    },
    decode: {
        '1B82': 0x02,
        '1B83': 0x03,
        '1B86': 0x06,
        '1B95': 0x15,
        '1B9B': 0x1B,
    },
};

/**
 * Apply substitution rules to decode a message.
 * @param {Buffer} data - The substituted message data (PDU-DATA or CKS).
 * @returns {Buffer} Decoded message.
 */
function applySubstitutionDecode(data) {
    const result = [];
    let i = 0;
    while (i < data.length) {
        if (data[i] === 0x1B && i + 1 < data.length) {
            const key = `1B${data[i + 1].toString(16).toUpperCase()}`;
            if (substitutionRules.decode[key]) {
                result.push(substitutionRules.decode[key]);
                i += 2; // Skip the substitution sequence
            } else {
                result.push(data[i]);
                i++;
            }
        } else {
            result.push(data[i]);
            i++;
        }
    }
    return Buffer.from(result);
}

/**
 * Decode a PDU-DATA message based on the specified structure.
 * @param {Buffer} data - Decoded PDU-DATA.
 * @returns {Object} Parsed fields or error details.
 */
function parsePDUData(data) {
    if (data.length < 13) {
        return { error: 'PDU-DATA is too short to parse.' };
    }

    return {
        messageType: data[0],
        nodeAddress: ((data[1] << 8) | data[2]),
        virtualDevice: data[3],
        objectId: ((data[4] << 16) | (data[5] << 8) | data[6]),
        parameterId: ((data[7] << 8) | data[8]),
        parameterValue: ((data[9] << 24) | (data[10] << 16) | (data[11] << 8) | data[12]),
    };
}

/**
 * Decode a PDU message based on the given protocol.
 * @param {Buffer} pdu - The entire PDU buffer.
 * @returns {Object} Decoded message or error details.
 */
function decodePDU(pdu) {
    try {
        // Validate STX and ETX
        if (pdu[0] !== 0x02 || pdu[pdu.length - 1] !== 0x03) {
            return { error: 'Invalid message start (STX) or end (ETX).' };
        }

        // Extract PDU-DATA and checksum
        const encodedData = pdu.subarray(1, pdu.length - 2); // PDU-DATA
        const encodedChecksum = pdu[pdu.length - 2]; // CKS

        // Decode PDU-DATA and checksum
        const pduData = applySubstitutionDecode(encodedData);
        const checksum = applySubstitutionDecode(Buffer.from([encodedChecksum]))[0];

        // Calculate checksum (XOR of all bytes in decoded PDU-DATA)
        const calculatedChecksum = pduData.reduce((acc, byte) => acc ^ byte, 0);
        if (checksum !== calculatedChecksum) {
            return { error: 'Checksum mismatch.', checksum, calculatedChecksum };
        }

        // Parse the PDU-DATA
        const parsedData = parsePDUData(pduData);
        return parsedData;
    } catch (error) {
        return { error: 'Failed to decode PDU.', details: error.message };
    }
}

/**
 * Send REST requests based on the result string.
 * @param {String} resultString - The result string generated from processed messages.
 */
async function sendRestRequest(resultString) {
    const baseUrl = `http://${PBX_ADDRESS}:${PBX_ARI_PORT}/ari/channels`;

    try {
        if (resultString) {
            // Send POST request
            const body = {
                endpoint: PBX_ENDPOINT,
                extension: PBX_EXTENSION,
                context: PBX_CONTEXT,
                priority: PBX_PRIORITY,
                channelId: PBX_CHANNEL_ID,
                variables: {
                    DESTINATIONS: resultString,
                    MAX_CALL_TIME: PBX_MAX_CALL_TIME,
                },
            };

            const response = await axios.post(baseUrl, body, {
                auth: {
                    username: PBX_USER,
                    password: PBX_PASSWORD,
                },
            });

            console.log('POST request successful:', response.data);
        } else {
            // Send DELETE request
            const deleteUrl = `${baseUrl}/${PBX_CHANNEL_ID}`;
            const response = await axios.delete(deleteUrl, {
                auth: {
                    username: PBX_USER,
                    password: PBX_PASSWORD,
                },
            });

            console.log('DELETE request successful:', response.data);
        }
    } catch (error) {
        console.error('REST request failed:', error.message);
    }
}

/**
 * Process all buffered messages.
 * This function is called after the timeout.
 */
async function processMessages() {
    console.log('Processing messages:');
    const processedMessages = messageBuffer.map(decodePDU).filter((msg) => !msg.error);

    console.log('Decoded Messages:', processedMessages);

    // Reference array with strings for parameter IDs
    const referenceArray = [
        "PJSIP/5090", // For parameterId = 0
        "PJSIP/5091", // For parameterId = 1
        "PJSIP/5092", // For parameterId = 2
        "PJSIP/5093", // For parameterId = 3
    ];

    // Generate result string
    const resultString = processedMessages
        .map(({ parameterId, parameterValue }) => (parameterValue > 0 ? referenceArray[parameterId] : null))
        .filter(Boolean)
        .join('&');

    console.log('Result String:', resultString);

    // Send REST request
    await sendRestRequest(resultString);

    // Clear buffer and timer
    messageBuffer = [];
    timer = null;
}

// Create a TCP server
const server = net.createServer((socket) => {
    console.log('Client connected');

    // Handle data received from client
    socket.on('data', (data) => {
        console.log('Received Data:', data);

        // Add received data (PDU) to the buffer
        messageBuffer.push(data);

        // Start or reset the timer
        if (timer) {
            clearTimeout(timer);
        }
        timer = setTimeout(() => {
            processMessages();
        }, TIMEOUT_DURATION);
    });

    // Handle client disconnect
    socket.on('end', () => {
        console.log('Client disconnected');
    });

    // Handle socket errors
    socket.on('error', (err) => {
        console.error('Socket error:', err.message);
    });
});

// Start the server on the configured port
server.listen(SERVER_PORT, () => {
    console.log(`TCP Server running on port ${SERVER_PORT}`);
});
