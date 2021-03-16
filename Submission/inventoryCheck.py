# Guide: https://www.tutorialspoint.com/send-mail-from-your-gmail-account-using-python
# https://retool.com/blog/your-guide-to-crud-in-firebase-realtimedb-with-rest-api/

import smtplib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
import requests
    
sender_address = 'trustcafeiot@gmail.com'
sender_pass = 'trustcafe'
receiver_address = 'trustcafeiot@gmail.com'


def send_mail(item, count):
    message = MIMEMultipart()
    message['From'] = sender_address
    message['To'] = receiver_address
    message['Subject'] = f'Inventory Alert! {item} is about to run out'
    mail_content = f'There are {str(count)} {item} left. Please purchase more'
    message.attach(MIMEText(mail_content, 'plain'))
    session = smtplib.SMTP('smtp.gmail.com', 587) #use gmail with port
    session.starttls() #enable security
    session.login(sender_address, sender_pass) #login with mail_id and password
    text = message.as_string()
    session.sendmail(sender_address, receiver_address, text)
    session.quit()

if __name__ == "__main__":
    headers = {
        'Content-type': 'application/json',
    }

    coffeeCountUrl = 'https://coffeeiot-c846f.firebaseio.com/-10.json?print=pretty'
    coffeeCount = requests.get(coffeeCountUrl).json()

    teaCountUrl = 'https://coffeeiot-c846f.firebaseio.com/-20.json?print=pretty'
    teaCount = requests.get(teaCountUrl).json()

    if coffeeCount < 10:
        send_mail("coffee", coffeeCount)

    if teaCount < 10:
        send_mail("tea", teaCount)
