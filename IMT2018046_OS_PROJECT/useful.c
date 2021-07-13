#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <pthread.h>
#include <fcntl.h>
struct user_info {
    int user_id;
    char type[20];
    char username[20];
    char password[20];
};

//struct details
struct account_info {
    float balance;
    int user_id;
};
int global_user_id = 0;

int signup(char*, char*, char*); 
void* handler(void*);
int delete_user(char* username){
    static struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;
    lock.l_pid = getpid();

    char filename[100];
    strcpy(filename, username);
    strncat(filename, ".txt", 5 * sizeof(char));
    int fd = open(filename, O_RDWR, 0644);

    if(fd == -1) {
        perror("Error"); 
        return -1;
    }
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("Error during file locking"); 
        return -1;
    }  
    return unlink(filename);
}

int modify_user(char* username, char* new_username, char* password) {
    struct user_info user;
    static struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_whence = SEEK_SET;
    lock.l_pid = getpid();

    char filename[100];
    strcpy(filename, username);
    strncat(filename, ".txt", 5 * sizeof(char));
    int fd = open(filename, O_RDWR, 0644);

    char* mode = malloc(10 * sizeof(char));
    if(fd == -1) {
        perror("Error"); 
        return -1;
    }
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("Error during file locking"); 
        return -1;
    }    
    // start of critical section
    lseek(fd, 0, SEEK_SET);
    if (read(fd, &user, sizeof(struct user_info)) == -1) { 
        perror("Error during read"); 
        return -1; 
    }
    delete_user(username);
    mode = !strcmp(user.type,"normal") ? "SIGNUP_U" : "SIGNUP_J";
    strcpy(user.username, new_username);
    signup(mode, new_username, password);
    // end of critical section
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);
    close(fd);
    return 0;
}

int check_user_type(char* user_type, char* mode) {
    if((strcmp(user_type,"normal") && !strcmp(mode, "SIGNIN_U")) 
        || (strcmp(user_type,"joint") && !strcmp(mode, "SIGNIN_J")) 
        || (strcmp(user_type,"admin") && !strcmp(mode, "SIGNIN_A")))    
            return -1;
    return 0;
}

int signup(char* mode, char* username, char* password){
    struct user_info user;
    struct account_info account;
    char filename[100];
    strcpy(filename, username);
    strncat(filename, ".txt", 5 * sizeof(char));
    int fd = open(filename, O_RDWR, 0644);

    if(fd != -1) {  // File (username) already exists
        // perror("Error"); 
        return -1;
    }
    else close(fd);
    fd = open(filename, O_WRONLY | O_CREAT,0644);
    if(fd == -1) {
        perror("Error"); 
        return -1;
    }
    user.user_id = global_user_id++;
    strcpy(user.username, username);
    strcpy(user.password, password);
    if (!strcmp(mode, "SIGNUP_U") || !strcmp(mode, "ADD_U")) strcpy(user.type,"normal");
    else if (!strcmp(mode, "SIGNUP_J")) strcpy(user.type,"joint");
    else if (!strcmp(mode, "SIGNUP_A")) strcpy(user.type,"admin");
    write(fd, &user, sizeof(struct user_info));
  
    account.balance = 0.0;
    account.user_id = user.user_id;
    write(fd, &account, sizeof(struct account_info));
    close(fd);
    return 0;
}

int signin(char* option, char* username, char* password){
    struct user_info user;
    static struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = sizeof(struct user_info);
    lock.l_pid = getpid();

    char filename[100];
    strcpy(filename, username);
    strncat(filename, ".txt", 5 * sizeof(char));
    int fd = open(filename, O_RDWR, 0644);
    if(fd == -1) {
        perror("Error"); 
        return -1;
    }
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("Error during file locking"); 
        return -1;
    }
    // start of critical section
    lseek(fd, 0, SEEK_SET);
    read(fd, &user, sizeof(struct user_info));

    if (check_user_type(user.type, option) == -1) return -1;
    if (strcmp(user.password, password)) return -1;
    // end of critical section
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);
    close(fd);
    return 0;
}

int deposit(char* username, float amount){
    struct account_info account;
    static struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_start = sizeof(struct user_info);
    lock.l_len = sizeof(struct account_info);
    lock.l_whence = SEEK_SET;
    lock.l_pid = getpid();

    char filename[100];
    strcpy(filename, username);
    strncat(filename, ".txt", 5 * sizeof(char));
    int fd = open(filename, O_RDWR, 0644);
    if(fd == -1) {
        perror("Error"); 
        return -1;
    }
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("Error during file locking"); 
        return -1;
    }
    // start of critical section
    lseek(fd, sizeof(struct user_info), SEEK_SET);
    if (read(fd, &account, sizeof(struct account_info)) == -1) {
        perror("Error during read"); 
        return -1;
    }
    printf("Balance = %f\n", account.balance);
    if (amount > 5000000) {
        printf("Cannot deposit more than 50L in one day\n");
        return -1;
    }
    account.balance += amount;
    lseek(fd, sizeof(struct user_info), SEEK_SET);
    if (write(fd, &account, sizeof(struct account_info)) == -1) {
        perror("Error during write"); 
        return -1;
    }
    // end of critical section
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);
    close(fd);
    return 0;
}

int withdraw(char* username, float amount){
    struct account_info account;
    static struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_start = sizeof(struct user_info);
    lock.l_len = sizeof(struct account_info);
    lock.l_whence = SEEK_SET;
    lock.l_pid = getpid();

    char filename[100];
    strcpy(filename, username);
    strncat(filename, ".txt", 5 * sizeof(char));
    int fd = open(filename, O_RDWR, 0644);
    if(fd == -1) {
        perror("Error"); 
        return -1;
    }
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("Error during file locking"); 
        return -1;
    }
    // start of critical section
    lseek(fd, sizeof(struct user_info), SEEK_SET);
    if (read(fd, &account, sizeof(struct account_info)) == -1) {
        perror("Error during read"); 
        return -1;
    }
    printf("Balance = %f\n", account.balance);
    if (account.balance < amount) return -1;
    account.balance -= amount;

    lseek(fd, sizeof(struct user_info), SEEK_SET);
    if (write(fd, &account, sizeof(struct account_info)) == -1) {
        perror("Error during write"); 
        return -1;
    }
    // end of critical section
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);
    close(fd);
    return 0;
}

float balance(char* username){
    struct account_info account;
    static struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_start = sizeof(struct user_info);
    lock.l_len = sizeof(struct account_info);
    lock.l_whence = SEEK_SET;
    lock.l_pid = getpid();

    char filename[10];
    strcpy(filename,username);
    strncat(filename, ".txt", 5 * sizeof(char));
    int fd = open(filename, O_RDONLY, 0644);
    if(fd == -1) {
        perror("Error"); 
        return -1;
    }
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("Error during file locking"); 
        return -1;
    }
    // start of critical section
    lseek(fd, sizeof(struct user_info), SEEK_SET);
    if (read(fd, &account, sizeof(struct account_info)) == -1) {
        perror("Error during read");
        return -1;
    }
    // end of critical section
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);
    close(fd);
    return account.balance;
}

int change_password(char* username, char* password){
    struct user_info user;
    char filename[100];

    static struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = sizeof(struct user_info);
    lock.l_pid = getpid();

    strcpy(filename, username);
    strncat(filename, ".txt", 5 * sizeof(char));
    int fd = open(filename, O_RDWR, 0644);
    if(fd == -1) {
        perror("Error"); 
        return -1;
    }
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("Error during file locking"); 
        return -1;
    }
    // start of critical section
    lseek(fd, 0, SEEK_SET);
    if (read(fd, &user, sizeof(struct user_info)) == -1) {
        perror("Error during read"); 
        return -1;
    }
    strcpy(user.password, password);
    lseek(fd, 0, SEEK_SET);
    if (write(fd, &user, sizeof(struct user_info)) == -1) {
        perror("Error during write"); 
        return -1;
    }
    // end of critical section
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);
    close(fd);
    return 0;
}

char* get_details(char* username){
    struct account_info account;
    struct user_info user;

    static struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;
    lock.l_pid = getpid();

    char filename[100];
    strcpy(filename, username);
    strncat(filename, ".txt", 5 * sizeof(char));
    int fd = open(filename, O_RDWR, 0644);
    if(fd == -1) {
        perror("Error"); 
        return "User does not exist\n";
    }
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("Error during file locking"); 
        return "Error: File is locked";
    }
    // start of critical section
    lseek(fd, 0, SEEK_SET);
    if (read(fd, &user, sizeof(struct user_info)) == -1 || read(fd, &account, sizeof(struct account_info))==-1) {
        perror("Error during read"); 
        return "Unable to read file\n";
    }
    // end of critical section
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);
    close(fd);
    char* return_string = (char*) malloc(100 * sizeof(char));
    sprintf(return_string, "Username: %s\nPassword: %s\nAccount Type: %s\nBalance: %f\n",
        user.username, user.password, user.type, account.balance);
    return return_string;
}
void *handler(void *socket_desc) {
	int socket = *(int*)socket_desc;
    int option, output;
	char* username = malloc(50 * sizeof(char));
	char* password = malloc(50 * sizeof(char));

	while(1){
		char* type = malloc(50 * sizeof(char));
		char* m = malloc(100 * sizeof(char));
		char* choice = malloc(50*sizeof(char));
		read(socket, choice, sizeof(choice)); 

        if (!strcmp(choice, "SIGNUP_U") || !strcmp(choice, "SIGNUP_J") || !strcmp(choice, "SIGNUP_A"))
         {
			read(socket, username, sizeof(username));
			read(socket, password, sizeof(password));
			int output = signup(choice, username, password);
			if(output == -1)
            {
            m = "User with this username already exists\n";
            }
            else 
            {m = "User authenticated successfully!\n";
            }
		}
        else if(!strcmp(choice, "SIGNIN_U") || !strcmp(choice, "SIGNIN_J") || !strcmp(choice, "SIGNIN_A"))
        {
            read(socket, username, sizeof(username));
			read(socket, password, sizeof(password));
			int output = signin(choice, username, password);
           if(output == -1)
            {
            m = "Authentication failed\n";
            }
            else 
            {m = "User authenticated successfully!\n";
            }
        }
		else if (!strcmp(choice, "DEPOSIT") || !strcmp(choice, "WITHDRAW")) {
	    	char* amount = malloc(10 * sizeof(char));
			read(socket, amount, sizeof(amount));
            int depos = !strcmp(choice, "DEPOSIT") ? 1 : 0;
            if(depos==1)
            {
                int output=deposit(username,atof(amount));
                if(output==-1)
                {
                    m="Cannot deposit more than 50L!\n" ;
                }
                else
                {
                    m="Amount deposited\n";
                }
            }
            else
            {
                int output=withdraw(username,atof(amount));
                if(output==-1)
                {
                    m="Not enough bank balance!\n" ;
                }
                else
                {
                    m="Amount withdrawn\n";
                }
            }
		}
		else if (!strcmp(choice, "BALANCE")) {
			sprintf(m, "%f", balance(username));
		}

		else if (!strcmp(choice, "PASSWORD")) {
			read(socket, password, sizeof(password));
			output = change_password(username, password);
			if (output == -1) m = "Error. Please try later.\n";
			else m = "Password changed\n";
		}

		else if (!strcmp(choice, "DETAILS")) {
			m = get_details(username);
		}

        else if (!strcmp(choice, "ADD_ACCOUNT")) {
			char* username = malloc(50 * sizeof(char));
			char* password = malloc(50 * sizeof(char));
			read(socket, type, sizeof(type));
			read(socket, username, sizeof(username));
			read(socket, password, sizeof(password));
			printf("User type: %s\n Username = %s\n Passowrd = %s\n", type, username, password);
            char* mode = malloc(20 * sizeof(char));
			if(!strcmp(type,"1")) 
                mode = "SIGNUP_U";  
			else mode = "SIGNUP_J";
			output = signup(mode, username, password);
			if(output == -1) m = "User already exists\n";
			else m = "User added!\n";
		}

        else if(!strcmp(choice, "MODIFY_ACCOUNT")) {
			char* username = malloc(50 * sizeof(char));
			char* password = malloc(50 * sizeof(char));
	    	char* new_username = malloc(50 * sizeof(char));
			read(socket, username, sizeof(username));
			read(socket, new_username, sizeof(new_username));
			read(socket, password, sizeof(password));
			output = modify_user (username, new_username,password);
			if (output == -1) m = "Cannot modify user.\n";
			else m = "User details modified!\n";
		}
		else if(!strcmp(choice, "DELETE_ACCOUNT")) {
			char* username = malloc(50 * sizeof(char));
			char* password = malloc(50 * sizeof(char));
			read(socket, username, sizeof(username));
			output = delete_user(username);
			if (output == -1) m = "User does not exist\n";
			else m = "User deleted!\n";
		}

		else if (!strcmp(choice, "GET_USER")) {
			char* username = malloc(50 * sizeof(char));
			read(socket, username, sizeof(username));
			m = get_details(username);
		}
		send(socket, m, 100 * sizeof(char), 0); 
	}
    return 0;
}

char* get_menu(char* menu){
	int option;

	if( !strcmp(menu, "Start")) {
		printf("Welcome to our online banking system. Please choose one of the two options:\n");
		printf("1. Sign Up\n");
		printf("2. Sign In\n");
		scanf("%d", &option); 
		if (option == 1) return "SIGN_UP";
		else if (option == 2) return "SIGN_IN";
		else return "INVALID";
	}

	else if( !strcmp(menu, "Sign Up")) {
		printf("What kind of account would you like to create?\n");
		printf("1. Normal User Account\n");
		printf("2. Joint User Account\n");
		printf("3. Admin Account\n");
		scanf("%d", &option);
		if (option == 1) return "SIGNUP_U";
		else if (option == 2) return "SIGNUP_J";
		else if (option == 3) return "SIGNUP_A";
		else return "INVALID";
	}

	else if( !strcmp(menu, "Sign In")) {
		printf("Choose one of the following sign in methods:\n");
		printf("1. Normal User Account\n");
		printf("2. Joint User Account\n");
		printf("3. Admin Account\n");
		scanf("%d", &option);
		if (option == 1) return "SIGNIN_U";
		else if (option == 2) return "SIGNIN_J";
		else if (option == 3) return "SIGNIN_A";
		else return "INVALID";
	}

	else if( !strcmp(menu, "Admin")) {
		printf("What can we do for you?\n");
		printf("1. Add a new user\n");
		printf("2. Delete an existing user\n");
		printf("3. Modify an existing user\n");
		printf("4. Search for a user's account details\n");
		printf("5. Exit\n") ;
		scanf("%d",&option);

		if (option == 1) return "ADD_ACCOUNT";
		else if (option == 2) return "DELETE_ACCOUNT";
		else if (option == 3) return "MODIFY_ACCOUNT";
		else if (option == 4) return "GET_ACCOUNT";
		else if (option == 5) return "EXIT";
		else return "INVALID";
	}
	else if( !strcmp(menu, "User")) {
		printf("What would you like to do?\n");
		printf("1. Deposit Money\n");
		printf("2. Withdraw Money\n");
		printf("3. Check Bank Balance\n");
		printf("4. Update Password\n");
		printf("5. View Account Details\n");
		printf("6. Exit\n");
		scanf("%d", &option);

		if (option == 1) return "DEPOSIT";
		else if (option == 2) return "WITHDRAW";
		else if (option == 3) return "BALANCE";
		else if (option == 4) return "PASSWORD";
		else if (option == 5) return "DETAILS";
		else if (option == 6) return "EXIT";
		else return "INVALID";
	}
} 

void user(int socket){
	char* choice = get_menu("User");
	char* option = malloc(10 * sizeof(char));
	char* amount = malloc(10 * sizeof(char));
	send(socket, choice, sizeof(option), 0); 

	if (!strcmp(choice, "EXIT")) exit(0);
	else if (!strcmp(choice, "DEPOSIT") || !strcmp(choice, "WITHDRAW")) {
		float amt;
		char* msg = !strcmp(choice, "DEPOSIT") ? "Deposit Amount" : "Withdraw Amount";
		printf("%s: ", msg);
		scanf("%f", &amt);
		sprintf(amount, "%f", amt);	
		send(socket, amount, sizeof(amount), 0);
	}
	else if (!strcmp(choice, "PASSWORD")) {
		char* password = malloc(50 * sizeof(char));
		printf("New password: ");
		scanf("%s", password);
		send(socket, password, sizeof(password), 0);
	}
	char* server_message = malloc(100*sizeof(char));
	read(socket, server_message, 100 * sizeof(char)); 
	printf("%s\n", server_message); 
}

void admin(int socket){
	char* choice = get_menu("Admin");
	char* option_string = malloc(10*sizeof(char));
	char* username = malloc(50 * sizeof(char));
	char* password = malloc(50 * sizeof(char));
	send(socket, choice, sizeof(option_string), 0); 

	if (!strcmp(choice, "EXIT")) exit(0);
	else if (!strcmp(choice, "ADD_ACCOUNT")) {
		int user_type;
		printf("Enter User Type \n1. Normal Account \n2. Joint Account\n");
		scanf("%d", &user_type);
		if (user_type != 1 && user_type != 2) {
			printf("Invalid type\n");
			exit(0);
		}
		printf("Username: ");
		scanf("%s", username);
		printf("Password: ");
		scanf("%s", password);
		if (user_type == 1) send(socket, "1", sizeof(char), 0);
		else if (user_type == 2) send(socket, "1", sizeof(char), 0); 
		send(socket, username, sizeof(username), 0); 
		send(socket, password, sizeof(password), 0); 
	}
	else if (!strcmp(choice, "MODIFY_ACCOUNT")) {
		char* new_username = malloc(50 * sizeof(char));
		printf("Username: ");
		scanf("%s", username);
		printf("Enter new username: ");
		scanf("%s", new_username);
		printf("Enter new password: ");
		scanf("%s", password);
		send(socket, username, sizeof(username), 0); 
		send(socket, new_username, sizeof(new_username), 0); 
		send(socket, password, sizeof(password), 0); 
	} 
	else if (!strcmp(choice, "DELETE_ACCOUNT") || !strcmp(choice, "GET_ACCOUNT")) {
		printf("Username: ");
		scanf("%s", username);
		send(socket, username, sizeof(username), 0); 
	}
	else {
		printf("Invalid input\n");
		exit(0);
	}
	char* server_message = malloc(100 * sizeof(char));
	read(socket, server_message, 100 * sizeof(char)); 
	printf("%s\n",server_message); 
}

