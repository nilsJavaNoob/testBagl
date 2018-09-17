public class Owner{
	private String name ="";
	
	public Owner(String name){
		this.name = name;
	}
	
	public String toString(){
		String result = "Owner\n" + name;
		
		/*for(Floor floor : floors){
			result+=floor.toString()+"\n";
		}*/
		return result;
	}
}//class