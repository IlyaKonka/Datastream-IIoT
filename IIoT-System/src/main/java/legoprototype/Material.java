package legoprototype;

import lombok.Getter;
import lombok.Setter;

@Getter
@Setter
public class Material {
    private String id;
    private String location;
    private boolean isRolled;
    private boolean isReadyToQCheck;
    private String quality;

    public Material(String id) {
        this.id = id;
        isRolled = false;
        isReadyToQCheck = false;
    }

    @Override
    public boolean equals(Object obj) {
        Material m = (Material) obj;
        return this.id.equals(m.id);
    }
}
