async function run() {
    const configuration = {'iceServers': [{'urls': 'stun:stun.l.google.com:19302'}]}
    const vid1 = document.getElementById('videoelement1')
    const vid2 = document.getElementById('videoelement2')
    const ws = new WebSocket('ws://localhost:8080')
    ws.addEventListener('error', console.error)
    const peerConnection = new RTCPeerConnection(configuration)
    try {
        const constraints = {'video': true, 'audio': true}
        const localStream = await navigator.mediaDevices.getUserMedia(constraints)
        localStream.getAudioTracks()[0].enabled = false
        vid1.srcObject = localStream
        localStream.getTracks().forEach(tr => peerConnection.addTrack(tr, localStream))
    } catch (e) {
        console.log("couldn't open camera")
    }
    
    peerConnection.onconnectionstatechange = event => {
        if (peerConnection.connectionState === 'connected') {
            console.log('connected')
        }
    }
    peerConnection.onicecandidate = event => {
        console.log('found candidate', event.candidate)
        if (event.candidate) {
            ws.send(JSON.stringify({ candidate: event.candidate }))
        }
    }
    peerConnection.ontrack = (event) => {
        console.log('ontrack')
        vid2.srcObject = event.streams[0]
    }
    ws.addEventListener('message', async e => {
        const message = JSON.parse(e.data)
        console.log('received ', message)
        if (message.offer) {
            peerConnection.setRemoteDescription(new RTCSessionDescription(message.offer))
            const answer = await peerConnection.createAnswer()
            await peerConnection.setLocalDescription(answer)
            ws.send(JSON.stringify({ answer }))
        }
        if (message.answer) {
            await peerConnection.setRemoteDescription(new RTCSessionDescription(message.answer))
        }
        if (message.candidate) {
            try {
                await peerConnection.addIceCandidate(new RTCIceCandidate(message.candidate))
            } catch (e) {
                console.error('Error adding received ice candidate', e)
            }
        }
    })
    document.getElementById('startbtn').addEventListener('click', async () => {
        const offer = await peerConnection.createOffer()
        await peerConnection.setLocalDescription(offer)
        ws.send(JSON.stringify({ offer }))
    })
    document.getElementById('playbtn').addEventListener('click', () => {
        vid2.play()
    })
}

run()





// const configuration = {'iceServers': [{'urls': 'stun:stun.l.google.com:19302'}]}
// const TURN_SERVER_URL = 'localhost:3478';
// const TURN_SERVER_USERNAME = 'username';
// const TURN_SERVER_CREDENTIAL = 'credential';
// const configuration = {
//     iceServers: [
//         {
//             urls: 'turn:' + TURN_SERVER_URL + '?transport=tcp',
//             username: TURN_SERVER_USERNAME,
//             credential: TURN_SERVER_CREDENTIAL
//         },
//         {
//             urls: 'turn:' + TURN_SERVER_URL + '?transport=udp',
//             username: TURN_SERVER_USERNAME,
//             credential: TURN_SERVER_CREDENTIAL
//         }
//     ]
// }



