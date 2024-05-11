from spyne import Application, rpc, ServiceBase, Unicode
from spyne.protocol.soap import Soap11
from spyne.server.wsgi import WsgiApplication
from datetime import datetime

class DateTimeService(ServiceBase):
    @rpc(_returns=Unicode)
    def get_datetime(ctx):
        now = datetime.now()
        formatted_datetime = now.strftime("%d/%m/%Y %H:%M:%S")
        return formatted_datetime

application = Application([DateTimeService], 'http://tests.python-zeep.org/',
                           in_protocol=Soap11(validator='lxml'),
                           out_protocol=Soap11())

application = WsgiApplication(application)

if __name__ == '__main__':
    import logging

    from wsgiref.simple_server import make_server
    logging.basicConfig(level=logging.DEBUG)
    logging.getLogger('spyne.protocol.xml').setLevel(logging.DEBUG)

    logging.info("listening to http://127.0.0.1:8000")
    logging.info("wsdl is at: http://localhost:8000/?wsdl")
    server = make_server('127.0.0.1', 8000, application)
    print("Servidor SOAP escuchando en http://127.0.0.1:8000")
    server.serve_forever()
    




