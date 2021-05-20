import java.awt.*;
import java.awt.event.*;
import java.io.IOException;
import java.lang.Process;
import java.lang.Runtime;


public class HelloWorld {
public static void main(String[] args) {

  //System.out.println("HelloWorld");

  String shpath="/home/lch/test";   //程序路径

  Process process =null;

  String command1 = "./waf --run scratch/scratch-simulator > simulator.out 2>&1";//"mkdir abcd";
  try{
        process = Runtime.getRuntime().exec(command1);
        
     }
  catch(IOException e1){
       e1.printStackTrace();
   }
  
  try{
       process.waitFor();
   }
  catch(InterruptedException e2){}

}
}
