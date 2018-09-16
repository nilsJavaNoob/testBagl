public class Appartment{
	
	private int appNmb = 0;
	private boolean status = true;
	
	public Appartment(int appNmb){
		this.appNmb = appNmb;
	}
	public boolean isFree(){
			return status;
	}
	
	public  void addOwner(Owner owner){
		//
	//	status = false;
	//	status = false;
	}
public String toString(){
	String result ="Appartment number - " + appNmb;
	return result;
}
}//class