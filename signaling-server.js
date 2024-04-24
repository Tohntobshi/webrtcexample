const ws = require('ws')

const serv = new ws.WebSocketServer({
  port: 8080
})


serv.on('connection', function connection(ws) {
    console.log('connected client')
    senderConnection = ws
    ws.on('error', console.error)
  
    ws.on('message', function message(d) {
      const data = JSON.parse(d);
      if (data.offer || data.answer || data.candidate) {
        for (const cl of serv.clients) {
          if (cl === ws) continue
          cl.send(JSON.stringify(data))
        }
      }
      console.log('received:', data)
    })
});
