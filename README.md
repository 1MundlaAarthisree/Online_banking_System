# Online_banking_System

This is an online banking system model. There are 3 types of users in the bank - Normal users, joint account users and admins. Each have their own roles.

Normal users: Users who can control only their account.

    Deposit amount
    withdraw amount
    Balance enquiry
    Password change
    View details
    Exit

Joint account users: Normal users whose account can also be controlled by another user. This is maintained by proper file locking. No two joint account users can’t write and read or write at a same time. However they can login and read together i.e., check balance.)

Admin: These people have extra privialges given by the bank. They have the access to

    Add user
    Delete user
    Modify User
    Get User Details

File Structure

constants.h Holds constant values for every user and admin commands. These are used throughout for the sake of readability. Also contains constants to indicate which option form the menu is chosen. The structs used to define user and account are also sepcified here.

client.c Program for client. This can be thought of as the "front end", and contains code for the user and admin interface. Messages are sent to the server via a socket.

server.c Program for a concurrent server. This can be thought of as the "back end". For every client, the server creates a new thread in order to service it further. This helps ensure smooth querying of server from various clients independetn of each other.

admin_commands.c Contains functions for commands exclusively used by admin. These include commands to delete and modify a user.

user_commands.c Contains functions for user commands. These implement file locking for the case of joint user.

Compilation: gcc server.c user_commands.c admin_commands.c -lpthread -o server gcc client.c -o client

Execution: ./server ./client
