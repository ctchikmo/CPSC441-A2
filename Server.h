#ifndef Server_H
#define Server_H

class User; // Forward declaration

// The Server class handles incomming connections and after a successful connection hands them off to a FileSender
class Server
{
	public:
		Server(int threads);
		~Server();
		
		void setUser(*User user);
		
		private:
			User* user;
};

#endif