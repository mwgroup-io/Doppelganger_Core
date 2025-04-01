#ifndef EMAIL_CONFIG_H
#define EMAIL_CONFIG_H

// Email configuration
extern bool enable_email;
extern char smtp_host[25];
extern char smtp_port[5];
extern char smtp_user[35];
extern char smtp_pass[20];
extern char smtp_recipient[35];

#endif // EMAIL_CONFIG_H