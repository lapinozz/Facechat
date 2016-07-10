# Facechat

A library to send and receive Facebook messages, or do anything that the official client can.
We do that by tricking Facebook into thinking we are simply a browser and copying https requests.

This project was inspired by the excellent [facebook-chat-api](https://github.com/Schmavery/facebook-chat-api) project.

More info on [my blog post](http://lapinozz.github.io/learning/2016/06/07/facechat-facebook-chat-api.html)

Usage example:

```cpp
Facechat f;
f.login("myemail@gmail.com", "password123");
if(f.login("monstrefou@gmail.com", "***REMOVED***"))
  std::cout << "login succes" << std::endl;
else
  std::cout << "login fail" << std::endl;
  
auto users = f.findUser("my friend's name");

if(users.empty())
  return -1;
  
auto id = users[0].id;

f.sendMessage("Hello!!", id);
```
