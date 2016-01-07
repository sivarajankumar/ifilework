## Description ##
<b>Ifilework</b> is a project to develop a client and server ftp in C language using sockets api for working with files.<br>
All participation is welcome, if you have any comments or suggestions on the current code, please send email to myfilesadev@gmail.com .<br>
<br>You can see the code  <a href='http://code.google.com/p/ifilework/source/browse/#svn%2Ftrunk'>here</a> .<br>
<br>
<h2>How to execute server</h2>
You can use the server in your computer at localhost address you must disable the firewall for localhost.<br>
The server has a program for managing users called ifw-mng.<br>
Run command :<br>
ifileworks -p 21 -d<br>

<h2>How to execute client</h2>
Run command :<br>
ifileworkc <br><br>

<blockquote><b>Client ifileworkc list of commands:</b><br><br>
<pre><code>  ?...............this helps<br>
  open &lt;server&gt; ..Connect to the server &lt;server&gt;<br>
  user............enter a login and password<br>
  bye.............exit the program<br>
  bin.............switch to binary mode<br>
  ascii...........switch to text mode<br>
  pwd.............display the current directory<br>
  cd &lt;dir&gt;........go to the directory &lt;dir&gt;<br>
  ls [&lt;dir&gt;]......display the contents of the directory&lt;dir&gt;<br>
  dir [&lt;dir&gt;].....display the contents of the directory &lt;dir&gt;<br>
  type &lt;file&gt;.....display the file &lt;file&gt;<br>
  get &lt;file&gt;......get file &lt;file&gt;<br>
  put &lt;file&gt;......send the file &lt;file&gt;<br>
  del &lt;file&gt;......delete the file &lt;file&gt;<br>
  syst............discover the system of the FTP server <br>
</code></pre></blockquote>



<h2>Version</h2>
0.0.1  ifilework client/server FTP (RFC959)