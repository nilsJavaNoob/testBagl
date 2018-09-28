public class Appartment{
	private int appartmentNumber;
	
	public Appartment(int appartmentNumber){
		System.out.println("Creating Appart " + appartmentNumber);
		this.appartmentNumber = appartmentNumber;
	}
	 public String toString(){
		String result = "App " + appartmentNumber;
		return result;
	} 
}//class