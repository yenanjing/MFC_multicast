# MFC_multicast
## multicast learning in network programming course.

A simple chat room in LAN implemented by MFC and multicast. It can send message and file to all members in multiscast group.

* **If you want to run this project on VS2017, you need to change the character set from Unicode(default) to multibyte character set.**

* **If you want to output the debug information, please open the project property and input "editbin /SUBSYSTEM:CONSOLE $(OUTDIR)\$(ProjectName).exe" in the command iine from the event generating.**

## Design ideas:
1. Considering the need for interface interaction, MFC was chosen to implement. For different data function transmission, I mainly use three non-blocking sockets to realize the functions of managing group, text transmission and file transmission respectively. Of course, you can also use a blocking socket to accept different types of data by adding flag bits to it, and depending on the flag bits, you can also use the socket to accept. I will introduce my code logic and design ideas from the use of three sockets.

2. Each user first needs to create a socketMC socket, join the multicast group, bind to a fixed port and address, send a message "J" to the fixed multicast address, and signal to other users to join the chat group. The socket is also set to asynchronous mode to listen for control messages from other users at any time. If you receive a "J" from another user, add their IP to your group List and display the List in the MFC main dialog. At the same time, send an "R" to the multicast address to indicate a reply. If you receive an "R" from another user, check to see if you have joined your group list, or if not, join. When a user wants to exit a multicast group, he sends a "Q" to the multicast address, and the member who receives the message removes the IP from his list. In addition, in order to ensure the correctness and timeliness of the group list, a Refresh button is set to clear the group list and send a "J" to the multicast address. The received users will reply with an "R", so as to get all active user information.

3. Then each user needs to create a socketMsg socket that also joins the multicast group and binds a fixed port and address, but the port is a different port from the socketMC. Set the socket to asynchronous mode. When a user clicks the SendMessage button, that socket sends their IP address and what the user typed in the Edit box, along with what you said in the chat log. The socket is also bound to an asynchronous message response function. When the port receives data, it receives a buffer, and the chat log displays the IP of the speaker and the text of the chat.

4. Ends with each user having to create a socketFile socket that also joins the multicast group, bound to a fixed port and address, and a different port from the first two. Sockets can be set to asynchronous mode, or blocking mode in the main process. When the user clicks the SendFile button, a call to GetOpenFileName pops up a dialog box to select a file. After selecting the file, the file name is sent to the multicast address, and a dialog box pops up, clicking OK to start transferring file data. The file is opened in RB mode, the Fopen function of C language, representing read-only. In the message response function, you need to determine whether the received data is a file name, and if so, a dialog box pops up to select save the file name and its path. If not, write the received data to the selected save file. Here, the file is opened in AB mode, indicating that it is appended to the end of the file in the same binary manner as the sender.


