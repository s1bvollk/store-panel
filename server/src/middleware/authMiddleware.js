const jws = require('jsonwebtoken')

module.exports = (req, res, next) => {
    if (req.method === "OPTIONS"){
        next()
    }
    try{
        const token = req.headers.authorization.split(' ')[1]
        if (!token) {
           return  res.status(401).json({message: "Не авторизован"})
        }
        const decoded = jws.verify(token, process.env.SECRET_KEY)
        req.user = decoded
        next()
    }catch (e) {
        res.status(401).json({message: "Не авторизован"})
    }
}