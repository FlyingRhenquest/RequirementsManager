# Setting up Postgres

Note: This covers setting up a local postgres database for a single admin user.
If you want to set up the node database to be used by several users or on a remote
server, the process will be a bit different. All the operations outside of the
CreateTables application only put node data in the tables and should not require
administrative database access to do that.

If you're an administrator, you should just be able to set the database up, create
a user for each user you want to connect and provide them with a .pgpass file
they can use to connect to the database.

# Install Postgres

## Linux (Debian-Type Systems)

Install Postgres client, server and dev libraries

    sudo apt install postgresql postgresql-client postgresql-server-dev-all

# Set your user up and create a database for them.

    sudo -u postgres psql
    CREATE USER your_username WITH ENCRYPTED PASSWORD 'your_password';
    CREATE DATABASE your_username;
    ALTER DATABASE your_username OWNER TO your_username;
    \c your_username
    GRANT USAGE ON SCHEMA public TO your_username;
    ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT ALL PRIVILEGES ON TABLES TO your_username;
    ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT ALL PRIVILEGES ON SEQUENCES TO your_username;
    ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT ALL PRIVILEGES ON FUNCTIONS TO your_username
    \q
    
    
Create a .pgpass file in your home directory with the format:

    localhost:5432:*:your_username:your_password
    
Set this file to user read/write only:

    chmod 600 .pgpass
    
Verify you can connect to your database:

    ~$psql
    psql (18.1 (Debian 18.1-2))
    Type "help" for help.
    \q

The libpqxx code should now use this file to connect to the database.

After you build this application, run the CreateTables program to create the tables it requires. You can safely run this program multiple times, but if you change a table format after creating the tables, you may have to manually alter the table format in your existing database if you want to preserve the data in that table. CreateTables will not re-define a database table once it has been created, it's just set up to idempotently create any missing tables.

If you run the RequirementsManager tests, they will currently insert nodes into the database but will not clean them up. The database also does not have referential integrity set up, so you can delete nodes out of one table without cleaning up node referencecs in other tables.

TODO: Write a test-cleanup function that will clean up UUIDs after test is done running.
