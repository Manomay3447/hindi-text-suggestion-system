import sys
import os


# Add the app and site-packages to sys.path
venv_path = "/var/www/hindi_suggestions/venv"
sys.path.insert(0, os.path.join(venv_path, "lib/python3.10/site-packages"))
sys.path.insert(0, "/var/www/hindi_suggestions")



from app import app as application
