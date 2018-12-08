<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<!--
    This is the main page of the bulletin board exercise from the
    course "Verteilte Computersysteme - TCP/IP" on the Technikum Wien.

    Author: Thomas M. Galla, Franz Hollerer
    Date: 2010-07-17
-->
<html>
  <head>
    <meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-15" />
    <meta http-equiv="refresh" content="5">
    <title>Verteilte Computersysteme - TCP/IP</title>
  </head>
  <body>
    <hr/>
    <center>
      <h1>Verteilte Computersysteme - TCP/IP</h1>
    </center>
    <hr/>

    <h2><a name="tcpibulletin"/>Bulletin Board</h2>

    <?php
      $fn="bulletin_board_content.dat";
      $file=fopen($fn, "r");
      flock($file, LOCK_SH);
      include($fn);
      flock($file, LOCK_UN);
      fclose($file);
    ?>
  </body>
</html>

