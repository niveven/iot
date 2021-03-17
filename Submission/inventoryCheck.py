# Guide: https://www.tutorialspoint.com/send-mail-from-your-gmail-account-using-python
# https://retool.com/blog/your-guide-to-crud-in-firebase-realtimedb-with-rest-api/

import smtplib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
import requests
    
sender_address = 'trustcafeiot@gmail.com'
sender_pass = 'trustcafe'
receiver_address = 'trustcafeiot@gmail.com'
base_url = 'https://coffeeiot-c846f.firebaseio.com/'
headers = {
    'Content-type': 'application/json',
}

def send_mail(subject, mail_content):
    message = MIMEMultipart()
    message['From'] = sender_address
    message['To'] = receiver_address
    message['Subject'] = subject
    content =  mail_content
    message.attach(MIMEText(mail_content, 'html'))
    session = smtplib.SMTP('smtp.gmail.com', 587) #use gmail with port
    session.starttls() #enable security
    session.login(sender_address, sender_pass) #login with mail_id and password
    text = message.as_string()
    session.sendmail(sender_address, receiver_address, text)
    session.quit()


def send_inventory_mail(item, count):
    send_mail(f'Inventory Alert! {item} is about to run out', f'There are only <b>{str(count)} {item}</b> left. Please purchase more<br />')

    
def check_coffee_tea_count():
    coffee_count_url = base_url + '-20.json?print=pretty' 
    coffee_count = requests.get(coffee_count_url).json()

    tea_count_url = base_url + '-10.json?print=pretty' 
    tea_count = requests.get(tea_count_url).json()

    if coffee_count < 50:
        send_inventory_mail("coffee", coffee_count)

    if tea_count < 50:
        send_inventory_mail("tea", tea_count)


def check_users():
    users_debt = {}
    users_legend_url = base_url + 'Users legend.json?print=pretty'
    users_json = requests.get(users_legend_url).json()
    users_list = [name for name_mail in users_json for name in name_mail.keys()]
    for i in range(0,len(users_list)):
        current_user_url = f'{base_url}{i}.json'
        current_user_money = requests.get(current_user_url).json()
        if current_user_money < 0:
            users_debt.update({users_list[i]: current_user_money})
    mail_content = f'The following users currently have debts:<br />'
    for user, money in users_debt.items():
        mail_content += f'User <b>{user}</b> owns {abs(money)} shekels<br />'
    send_mail('Users Debt', mail_content)


if __name__ == "__main__":
    check_coffee_tea_count();
    check_users();

    