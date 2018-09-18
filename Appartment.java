public class Appartment{
	
	private int appNmb = 0;
	private Owner[] owners;
	private boolean status = true;
	
	public Appartment(int appNmb,int capasity){
		this.appNmb = appNmb;
		owners = new Owner[capasity];
	}
	
	public String toString(){
	String result ="Appartment number - " + appNmb + "\n";
	/*for(Owner owner : owners ){
		result += owner.toString()+ "/n";
	}*/
	return result;
	}
//==================================
	public boolean isFree(){
			//System.out.println("Array size is " + owners.length);
			//return owners[owners.length-1 == null];
			return false;
	}
	
	public  void addOwner(Owner owner){
		owners[getFreeRoomIndex()] = owner;
	}
	public int getFreeRoomIndex(){
		for(int i = 0; i < owners.length; i++){
			//if(owners[i] )
		}
		return 5;
	}

}//class