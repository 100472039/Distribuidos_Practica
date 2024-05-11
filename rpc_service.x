program SERVER {
    version SERVER_VERS {
        
        int REGISTER(string, string, string) = 1;
        int UNREGISTER(string, string, string) = 2;
        int CONNECT(string, string, string) = 3;
        int DISCONNECT(string, string, string) = 4;
        int PUBLISH(string, string, string, string) = 5;
        int DELETE(string, string, string, string) = 6;
        int LIST_USERS(string, string, string) = 7;
        int LIST_CONTENT(string, string, string) = 8; 

    } = 1;
} = 0x31230000;
